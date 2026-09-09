// link_test.cpp - Tests du linker : des objets, une image
//
// Le gain de test de cette suite est qu'elle part d'objets FABRIQUES A LA MAIN.
// Verifier un recouvrement, une banque derivee ou un point d'entree ne demande
// plus d'ecrire un source Z80 qui les provoque, mais deux structures de dix
// lignes — et un test qui echoue nomme le linker, pas l'assembleur.
#include "link.h"

#include "profile.h"
#include "script.h"

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

static int g_pass = 0, g_fail = 0;

static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

static std::string hex(const std::vector<uint8_t> &v) {
    std::string s; char b[8];
    for (size_t i = 0; i < v.size(); ++i) { snprintf(b, sizeof b, "%02X", v[i]); if (i) s += ' '; s += b; }
    return s;
}

static void okBytes(const char *desc, const std::vector<uint8_t> &got,
                    std::initializer_list<uint8_t> want) {
    const std::vector<uint8_t> exp(want);
    if (got == exp) ++g_pass;
    else {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s\n    attendu [%s]\n    obtenu  [%s]\n",
               desc, hex(exp).c_str(), hex(got).c_str());
    }
}

// Un fragment ECRIT en entier par le site `site`, pose a `addr`.
static asmb::Fragment frag(int addr, std::initializer_list<uint8_t> bytes, uint16_t site = 1) {
    asmb::Fragment f;
    f.placed = true;
    f.addr = addr;
    f.logical = addr;
    f.bytes = std::vector<uint8_t>(bytes);
    f.prov.assign(f.bytes.size(), site);
    return f;
}

// Un objet d'un seul fragment, avec une table de sites d'une ligne.
static asmb::Object obj1(const asmb::Fragment &f, const char *file = "a.asm", int line = 1) {
    asmb::Object o;
    o.sites.push_back({file, line});
    o.fragments.push_back(f);
    return o;
}

// Un objet dont chaque entree est (nom de section, octets) : une section
// relocalisable "RO", un fragment, dans l'ordre donne. C'est tout ce qu'il faut
// pour exercer le placement des sections FUSIONNEES — et cela tient en une
// ligne par unite, ce qui est le gain de test de cette suite.
static asmb::Object secObj(const char *unit,
                           std::vector<std::pair<std::string, std::vector<uint8_t>>> secs,
                           const char *file = nullptr, int line = 1) {
    asmb::Object o;
    o.name = unit;
    o.sites.push_back({file ? file : unit, line});
    int id = 0;
    for (auto &p : secs) {
        asmb::Fragment f;
        f.placed = false;
        f.relocSection = id;
        f.section = p.first;
        f.bytes = p.second;
        f.prov.assign(f.bytes.size(), 1);
        o.fragments.push_back(f);
        asmb::Section sec;
        sec.name = p.first;
        sec.id = id;
        sec.relocatable = true;
        sec.kind = "RO";
        sec.size = (int64_t)p.second.size();
        sec.file = file ? file : unit;
        sec.line = line;
        o.sections.push_back(sec);
        ++id;
    }
    return o;
}

int main() {
    printf("Tests linkage (objets -> image)\n");

    // --- Placement absolu ---------------------------------------------------
    {
        link::Image img = link::build({obj1(frag(0x8000, {1, 2, 3}))});
        ok("un fragment va ou son org le dit", img.ok && img.loadAddress == 0x8000);
        okBytes("ses octets sortent tels quels", img.bin, {1, 2, 3});
        ok("un bloc, une banque", img.blocks.size() == 1 && img.blocks[0].bank == 2);
        ok("le bloc porte sa coverage", img.blocks[0].covered == std::vector<uint8_t>{1, 1, 1});
        ok("la banque ecrite est nommee", img.banksWritten == std::vector<int>{2});
    }
    {
        // Deux fragments disjoints : le binaire est l'intervalle qui les couvre,
        // et le creux entre eux vaut zero sans etre couvert.
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {0xAA}));
        o.fragments.push_back(frag(0x8003, {0xBB}));
        link::Image img = link::build({o});
        okBytes("l'intervalle couvre les deux", img.bin, {0xAA, 0, 0, 0xBB});
        link::Flat flat = link::flatten(img);
        ok("le creux n'est pas couvert", !flat.covered[0x8001] && !flat.covered[0x8002]);
        ok("les deux extremites le sont", flat.covered[0x8000] && flat.covered[0x8003]);
    }

    // --- Un trou reserve n'est ni ecrit ni couvert ---------------------------
    {
        asmb::Fragment f = frag(0x8000, {1, 0, 3});
        f.prov[1] = 0;   // un `ds` au milieu du fragment
        link::Image img = link::build({obj1(f)});
        link::Flat flat = link::flatten(img);
        ok("le trou n'est pas couvert", flat.covered[0x8000] && !flat.covered[0x8001] &&
                                        flat.covered[0x8002]);
        ok("mais il compte dans l'intervalle", img.bin.size() == 3);
    }

    // --- Recouvrement -------------------------------------------------------
    {
        asmb::Object o;
        o.sites.push_back({"a.asm", 2});   // site 1
        o.sites.push_back({"a.asm", 4});   // site 2
        o.fragments.push_back(frag(0x8000, {1, 2, 3, 4}, 1));
        o.fragments.push_back(frag(0x8001, {9, 9}, 2));
        link::Image img = link::build({o});
        ok("un seul avertissement pour la plage", img.warnings.size() == 1);
        const std::string m = img.warnings.empty() ? std::string() : img.warnings[0].message;
        ok("la plage est coalescee", m.find("&8001-&8002") != std::string::npos);
        ok("il nomme le site ecrase", m.find("a.asm:2") != std::string::npos);
        ok("il est rapporte sur le site ecrasant",
           !img.warnings.empty() && img.warnings[0].line == 4);
        okBytes("le dernier ecrit gagne", img.bin, {1, 9, 9, 4});
    }
    {
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {1}));
        o.fragments.push_back(frag(0x9000, {1}));
        link::Image img = link::build({o});
        ok("deux fragments disjoints n'en produisent pas", img.warnings.empty());
    }
    {
        // ENTRE DEUX OBJETS, c'est un REFUS et non un avertissement. A
        // l'interieur d'un fichier, reecrire est un idiome que l'auteur voit ;
        // entre deux unites assemblees separement, personne ne l'a voulu et
        // personne ne le verrait. Et le refus REMPLACE l'avertissement : deux
        // diagnostics pour un seul fait en valent zero.
        asmb::Object a = obj1(frag(0x8000, {1, 2}), "a.asm", 7);
        a.name = "a.fo";
        asmb::Object b = obj1(frag(0x8000, {3, 4}), "b.asm", 9);
        b.name = "b.fo";
        link::Image img = link::build({a, b});
        ok("un recouvrement inter-objets est refuse", !img.ok && img.errors.size() == 1);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("il nomme les DEUX objets",
           m.find("'a.fo'") != std::string::npos && m.find("'b.fo'") != std::string::npos);
        ok("et l'adresse en conflit", m.find("&8000") != std::string::npos);
        ok("il est rapporte sur la ligne qui ecrase",
           !img.errors.empty() && img.errors[0].file == "b.asm" && img.errors[0].line == 9);
        ok("et il ne double pas l'avertissement de chevauchement", img.warnings.empty());
    }
    {
        // Deux objets qui exportent le meme nom : refuse, en nommant LES DEUX
        // provenances. En choisir un ferait dependre le programme de l'ordre
        // des fichiers sur la ligne de commande.
        asmb::Object a = obj1(frag(0x8000, {1}), "a.asm", 1);
        a.name = "a.fo";
        asmb::Symbol sa; sa.name = "shared"; sa.isPublic = true; sa.value = 0x8000; sa.frag = 0; sa.line = 3;
        a.symbolTable.push_back(sa);
        asmb::Object b = obj1(frag(0x9000, {2}), "b.asm", 1);
        b.name = "b.fo";
        asmb::Symbol sb; sb.name = "shared"; sb.isPublic = true; sb.value = 0x9000; sb.frag = 0; sb.line = 5;
        b.symbolTable.push_back(sb);
        link::Image img = link::build({a, b});
        ok("un symbole exporte deux fois est refuse", !img.ok && img.errors.size() == 1);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et les deux provenances sont nommees",
           m.find("'a.fo'") != std::string::npos && m.find("'b.fo'") != std::string::npos &&
           m.find("shared") != std::string::npos);
    }
    {
        // N objets se linkent, et un EXTERN se resout contre le PUBLIC d'un
        // autre : c'est la compilation separee, reellement livree.
        asmb::Object a;
        a.name = "a.fo";
        a.sites.push_back({"a.asm", 4});
        asmb::Fragment fa = frag(0, {0xCD, 0, 0});
        fa.placed = false; fa.relocSection = 0;
        a.fragments.push_back(fa);
        asmb::Section seca; seca.name = "code"; seca.id = 0; seca.relocatable = true;
        a.sections.push_back(seca);
        asmb::Reloc r; r.frag = 0; r.offset = 1; r.kind = asmb::Reloc::Abs16; r.symbol = "draw";
        a.relocs.push_back(r);

        asmb::Object b;
        b.name = "b.fo";
        b.sites.push_back({"b.asm", 3});
        asmb::Fragment fb = frag(0, {0xC9});
        fb.placed = false; fb.relocSection = 0;
        b.fragments.push_back(fb);
        asmb::Section secb; secb.name = "lib"; secb.id = 0; secb.relocatable = true;
        b.sections.push_back(secb);
        asmb::Symbol sd; sd.name = "draw"; sd.isPublic = true; sd.frag = 0; sd.offset = 0;
        b.symbolTable.push_back(sd);

        link::Image img = link::build({a, b});
        ok("deux objets se linkent", img.ok);
        okBytes("et l'EXTERN pointe sur le PUBLIC de l'autre", img.bin, {0xCD, 0x03, 0x00, 0xC9});
    }
    {
        // Un EXTERN que personne n'exporte est refuse, en nommant le symbole ET
        // l'objet qui le demande.
        asmb::Object a;
        a.name = "a.fo";
        a.sites.push_back({"a.asm", 4});
        a.fragments.push_back(frag(0x8000, {0xCD, 0, 0}));
        asmb::Reloc r; r.frag = 0; r.offset = 1; r.kind = asmb::Reloc::Abs16; r.symbol = "draw";
        a.relocs.push_back(r);
        link::Image img = link::build({a});
        ok("un EXTERN sans definition est refuse", !img.ok && img.errors.size() == 1);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("il nomme le symbole et l'objet demandeur",
           m.find("'draw'") != std::string::npos && m.find("'a.fo'") != std::string::npos);
    }

    // --- Banques ------------------------------------------------------------
    {
        // Sans prefixe, la banque SUIT l'adresse : un fragment qui franchit une
        // frontiere de 16 K donne deux blocs, dans deux banques.
        link::Image img = link::build({obj1(frag(0x3FFE, {1, 2, 3, 4}))});
        ok("deux blocs de part et d'autre de la frontiere", img.blocks.size() == 2);
        ok("le premier est en banque 0",
           img.blocks.size() == 2 && img.blocks[0].bank == 0 && img.blocks[0].bytes.size() == 2);
        ok("le second en banque 1",
           img.blocks.size() == 2 && img.blocks[1].bank == 1 && img.blocks[1].addr == 0x4000);
        ok("les deux banques sont nommees", img.banksWritten == std::vector<int>{0, 1});
    }
    {
        // Un prefixe FIXE la banque du fragment, quelle que soit son adresse.
        asmb::Fragment f = frag(0x4000, {0xAB});
        f.bank = 5;
        link::Image img = link::build({obj1(f)});
        ok("le prefixe fixe la banque", img.banksWritten == std::vector<int>{5});
        link::Flat flat = link::flatten(img);
        ok("l'octet est en banque 5, offset 0", flat.bytes[5 * 0x4000] == 0xAB);
        ok("la coverage suit", flat.covered[5 * 0x4000] != 0);
        ok("et rien a l'adresse nue", !flat.covered[0x4000]);
        ok("hors des 64 K de base, pas de binaire plat", img.bin.empty());
    }
    {
        // Au-dela de la banque 7, aucun dump plat ne peut porter les octets. Le
        // linker les POSE quand meme et le dit par `banksWritten` : c'est au CLI
        // de refuser l'export, pas au linkage d'echouer.
        asmb::Fragment f = frag(0, {0xAB});
        f.bank = 9;
        link::Image img = link::build({obj1(f)});
        ok("la banque haute est nommee", img.ok && img.banksWritten == std::vector<int>{9});
        link::Flat flat = link::flatten(img);
        ok("mais l'image plate ne la porte pas",
           flat.bytes.size() == (size_t)link::kFlatBanks * 0x4000);
    }

    // --- Point d'entree -----------------------------------------------------
    {
        link::Image img = link::build({obj1(frag(0x8000, {1, 2}))});
        ok("sans run, l'entree est la premiere adresse ecrite", img.runAddress == 0x8000);
    }
    {
        asmb::Object o = obj1(frag(0x8000, {1, 2}));
        o.entry.has = true;
        o.entry.value = 0x1234;
        link::Image img = link::build({o});
        ok("une adresse litterale voyage telle quelle", img.runAddress == 0x1234);
    }
    {
        // Un NOM est resolu ICI : c'est le linker qui connait l'adresse
        // definitive, et c'est ce qui permettra a `run` de viser un label d'une
        // section relocalisable.
        asmb::Object o = obj1(frag(0x8000, {1, 2}));
        o.symbols["main"] = 0x8001;
        o.entry.has = true;
        o.entry.name = "main";
        o.entry.value = 0;   // l'assembleur n'a pas a le savoir
        link::Image img = link::build({o});
        ok("un nom est resolu par le linker", img.runAddress == 0x8001);
    }
    {
        // Un bloc DEPLACE range ses octets ailleurs que leur adresse logique :
        // demarrer dessus ferait demarrer sur de la memoire vide.
        asmb::Fragment f = frag(0x3000, {1, 2, 3});
        f.logical = 0x2000;
        asmb::Object o = obj1(f);
        o.entry.has = true;
        o.entry.value = 0x2001;
        link::Image img = link::build({o});
        ok("run dans un bloc deplace est signale", img.warnings.size() == 1 &&
            img.warnings[0].message.find("displaced") != std::string::npos);
        ok("et le PC reste celui que la source a demande", img.runAddress == 0x2001);
    }
    {
        asmb::Fragment f = frag(0x3000, {1, 2, 3});
        f.logical = 0x2000;
        asmb::Object o = obj1(f);
        o.entry.has = true;
        o.entry.value = 0x3000;   // l'adresse de RANGEMENT, elle, porte bien les octets
        link::Image img = link::build({o});
        ok("run sur le rangement ne l'est pas", img.warnings.empty());
    }

    // --- Table des symboles (amendement a l'ADR 0019) -----------------------
    // Elle sort du LINKER : un label vaut l'adresse de son fragment plus son
    // offset, et l'assembleur ne connait que le second terme.
    {
        asmb::Object o = obj1(frag(0x8000, {1, 2, 3}));
        asmb::Symbol s;
        s.name = "milieu"; s.value = 0x8001; s.frag = 0; s.offset = 1; s.section = "code";
        s.file = "a.asm"; s.line = 3;
        o.symbolTable.push_back(s);
        link::Image img = link::build({o});
        ok("un symbole, une entree", img.symbolTable.size() == 1);
        const link::Symbol &r = img.symbolTable[0];
        ok("le rangement est derive du fragment", r.store == 0x8001);
        ok("la banque aussi", r.bank == 2);
        ok("la provenance traverse", r.file == "a.asm" && r.line == 3 && r.section == "code");
    }
    {
        // Un bloc deplace : le RANGEMENT n'est pas l'adresse logique, et c'est
        // toute la raison d'avoir deux colonnes.
        asmb::Fragment f = frag(0x3000, {1, 2});
        f.logical = 0x2000;
        asmb::Object o = obj1(f);
        asmb::Symbol s;
        s.name = "ici"; s.value = 0x2000; s.frag = 0; s.offset = 0;
        o.symbolTable.push_back(s);
        link::Image img = link::build({o});
        ok("la valeur reste l'adresse logique", img.symbolTable[0].value == 0x2000);
        ok("le rangement est celui du fragment", img.symbolTable[0].store == 0x3000);
    }
    {
        // Un prefixe de banque : le symbole la porte, sans que son adresse la dise.
        asmb::Fragment f = frag(0x4000, {0xAB});
        f.bank = 5;
        asmb::Object o = obj1(f);
        asmb::Symbol s;
        s.name = "haut"; s.value = 0x4000; s.frag = 0; s.offset = 0;
        o.symbolTable.push_back(s);
        link::Image img = link::build({o});
        ok("la banque vient du fragment, pas de l'adresse",
           img.symbolTable[0].bank == 5 && img.symbolTable[0].store == 0x4000);
    }
    {
        // Une constante n'habite nulle part : ni banque, ni rangement.
        asmb::Object o = obj1(frag(0x8000, {1}));
        asmb::Symbol s;
        s.name = "TAILLE"; s.isConst = true; s.value = 42;
        o.symbolTable.push_back(s);
        link::Image img = link::build({o});
        ok("une constante n'a ni banque ni rangement",
           img.symbolTable[0].bank == -1 && img.symbolTable[0].store == -1 &&
           img.symbolTable[0].value == 42);
    }

    // --- Sections relocalisables et relocalisations -------------------------
    // Un fragment d'une section relocalisable porte son OFFSET dans `addr` : le
    // linker y ajoute la base qu'il decide.
    {
        asmb::Fragment f = frag(0, {1, 2, 3});
        f.placed = false;
        f.relocSection = 0;
        asmb::Object o = obj1(f);
        asmb::Section sec; sec.name = "data"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        link::Image img = link::build({o});
        ok("sans rien d'absolu, elle est posee a zero", img.loadAddress == 0);
        okBytes("ses octets sortent tels quels", img.bin, {1, 2, 3});
    }
    {
        // Avec un bloc absolu, la section relocalisable se pose APRES lui.
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {0xAA, 0xBB}));
        asmb::Fragment r = frag(0, {1, 2});
        r.placed = false; r.relocSection = 0;
        o.fragments.push_back(r);
        asmb::Section sec; sec.name = "data"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        link::Image img = link::build({o});
        okBytes("elle suit le dernier octet absolu", img.bin, {0xAA, 0xBB, 1, 2});
        ok("et rien ne se recouvre", img.warnings.empty());
    }
    {
        // Abs16 : le linker ECRIT l'adresse finale, il ne l'additionne pas a ce
        // qui s'y trouve. Un objet dont l'addend est faux se lit a l'oeil.
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {0x21, 0xFF, 0xFF}));   // ld hl,????
        asmb::Fragment r = frag(0, {9, 9});
        r.placed = false; r.relocSection = 0;
        o.fragments.push_back(r);
        asmb::Section sec; sec.name = "data"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        asmb::Reloc rel;
        rel.frag = 0; rel.offset = 1; rel.kind = asmb::Reloc::Abs16; rel.section = 0; rel.addend = 1;
        o.relocs.push_back(rel);
        link::Image img = link::build({o});
        okBytes("Abs16 ecrit base + addend, petit-boutien", img.bin, {0x21, 0x04, 0x80, 9, 9});
    }
    {
        // High8 et Low8 : les deux octets d'une adresse qu'on ne connaissait pas.
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {0, 0}));
        asmb::Section sec; sec.name = "data"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        asmb::Fragment r = frag(0, {7});
        r.placed = false; r.relocSection = 0;
        o.fragments.push_back(r);
        asmb::Reloc hi; hi.frag = 0; hi.offset = 0; hi.kind = asmb::Reloc::High8; hi.section = 0; hi.addend = 0;
        asmb::Reloc lo; lo.frag = 0; lo.offset = 1; lo.kind = asmb::Reloc::Low8;  lo.section = 0; lo.addend = 0;
        o.relocs.push_back(hi);
        o.relocs.push_back(lo);
        link::Image img = link::build({o});
        okBytes("High8 puis Low8", img.bin, {0x80, 0x02, 7});
    }
    {
        // Rel8 : le deplacement se compte depuis l'octet SUIVANT celui qui le
        // porte. Ici, et nulle part ailleurs, les deux adresses sont connues.
        asmb::Object o;
        o.sites.push_back({"a.asm", 5});
        o.fragments.push_back(frag(0x8000, {0x18, 0x00}));   // jr ????
        asmb::Fragment r = frag(0, {0xC9});
        r.placed = false; r.relocSection = 0;
        o.fragments.push_back(r);
        asmb::Section sec; sec.name = "code"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        asmb::Reloc rel;
        rel.frag = 0; rel.offset = 1; rel.kind = asmb::Reloc::Rel8; rel.section = 0; rel.addend = 0;
        o.relocs.push_back(rel);
        link::Image img = link::build({o});
        okBytes("Rel8 compte depuis l'octet suivant", img.bin, {0x18, 0x00, 0xC9});
    }
    {
        // Hors de portee INTER-sections : c'est le linker qui refuse, parce que
        // lui seul connait la distance.
        asmb::Object o;
        o.sites.push_back({"a.asm", 5});
        o.fragments.push_back(frag(0x8000, {0x18, 0x00}));
        asmb::Fragment r = frag(0, {0xC9});
        r.placed = false; r.relocSection = 0;
        r.addr = 0;
        o.fragments.push_back(r);
        asmb::Section sec; sec.name = "code"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        asmb::Reloc rel;
        rel.frag = 0; rel.offset = 1; rel.kind = asmb::Reloc::Rel8; rel.section = 0; rel.addend = 300;
        o.relocs.push_back(rel);
        link::Image img = link::build({o});
        ok("une portee inter-sections hors bornes est refusee", !img.ok && img.errors.size() == 1);
        ok("et elle nomme la ligne qui l'a ecrite",
           !img.errors.empty() && img.errors[0].file == "a.asm" && img.errors[0].line == 5);
    }
    {
        // La table des symboles suit la base : c'est tout l'objet de l'amendement.
        asmb::Object o;
        o.sites.push_back({"a.asm", 1});
        o.fragments.push_back(frag(0x8000, {0xAA}));
        asmb::Fragment r = frag(0, {1, 2});
        r.placed = false; r.relocSection = 0;
        o.fragments.push_back(r);
        asmb::Section sec; sec.name = "data"; sec.id = 0; sec.relocatable = true;
        o.sections.push_back(sec);
        asmb::Symbol sy;
        sy.name = "tbl"; sy.value = 1; sy.frag = 1; sy.offset = 1; sy.section = "data";
        o.symbolTable.push_back(sy);
        link::Image img = link::build({o});
        ok("le symbole vaut la base plus son offset",
           img.symbolTable[0].value == 0x8002 && img.symbolTable[0].store == 0x8002);
    }

    // --- C1.0 : la section fusionnee par nom --------------------------------
    {
        // Deux objets declarant chacun DEUX sections. Les contributions d'un
        // meme nom se suivent : c'est ce qui permet de remplir une ROM avec les
        // sections "ro" de dix fichiers (§11). Avant C1.0, l'ordre etait
        // a1 b1 a2 b2 — chaque objet recevait ses propres bases.
        link::Image img = link::build({secObj("a.fo", {{"a", {1, 2}}, {"b", {3}}}),
                                       secObj("b.fo", {{"a", {4}}, {"b", {5, 6}}})});
        ok("deux objets, deux sections : le placement est vert", img.ok);
        okBytes("les contributions d'un meme nom se suivent", img.bin, {1, 2, 4, 3, 5, 6});
    }
    {
        // Un seul nom partage : les deux unites remplissent LA MEME section, et
        // la seconde commence ou la premiere s'arrete.
        link::Image img = link::build({secObj("a.fo", {{"code", {1, 2}}}),
                                       secObj("b.fo", {{"code", {3}}})});
        ok("un nom, une section", img.ok);
        okBytes("la seconde unite suit la premiere", img.bin, {1, 2, 3});
    }
    {
        // L'ETENDUE, et non les octets emis : un trou reserve occupe la place, et
        // la contribution suivante commence apres.
        asmb::Object a = secObj("a.fo", {{"code", {1, 0, 0}}});
        a.fragments[0].prov[1] = 0;   // un `ds` de deux octets
        a.fragments[0].prov[2] = 0;
        link::Image img = link::build({a, secObj("b.fo", {{"code", {9}}})});
        okBytes("un trou reserve n'est pas de la place libre", img.bin, {1, 0, 0, 9});
    }
    {
        // Le type est fige par la premiere declaration A TRAVERS les objets :
        // rouvrir en "rw" ce qu'une autre unite a declare "ro" desarmerait le
        // refus d'ecriture en ROM en silence.
        asmb::Object a = secObj("a.fo", {{"data", {1}}}, "a.asm", 7);
        asmb::Object b = secObj("b.fo", {{"data", {2}}}, "b.asm", 9);
        b.sections[0].kind = "RW";
        link::Image img = link::build({a, b});
        ok("un type qui change d'une unite a l'autre est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("il nomme les deux unites",
           m.find("'a.fo'") != std::string::npos && m.find("'b.fo'") != std::string::npos);
        ok("et les deux types",
           m.find("\"ro\"") != std::string::npos && m.find("\"rw\"") != std::string::npos);
        ok("rapporte sur la SECONDE declaration, la seule que son auteur peut changer",
           !img.errors.empty() && img.errors[0].file == "b.asm" && img.errors[0].line == 9);
    }
    {
        // Deux plafonds differents. Retenir le plus petit serait defendable, et
        // c'est la raison de refuser : personne ne pourrait deviner laquelle des
        // deux lectures a ete appliquee.
        asmb::Object a = secObj("a.fo", {{"blob", {1}}}, "a.asm", 3);
        asmb::Object b = secObj("b.fo", {{"blob", {2}}}, "b.asm", 4);
        a.sections[0].hasMax = true; a.sections[0].max = 0x2000;
        b.sections[0].hasMax = true; b.sections[0].max = 0x1000;
        link::Image img = link::build({a, b});
        ok("deux plafonds differents sont refuses", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("les deux valeurs sont nommees",
           m.find("0x2000") != std::string::npos && m.find("0x1000") != std::string::npos);
    }
    {
        // Un plafond d'un cote, aucun de l'autre : meme refus. Un plafond qu'une
        // seule unite declare est un plafond qu'un `include` peut faire
        // disparaitre sans un mot.
        asmb::Object a = secObj("a.fo", {{"blob", {1}}}, "a.asm", 3);
        asmb::Object b = secObj("b.fo", {{"blob", {2}}}, "b.asm", 4);
        a.sections[0].hasMax = true; a.sections[0].max = 0x2000;
        link::Image img = link::build({a, b});
        ok("un plafond d'un seul cote est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le refus dit laquelle n'en declare pas", m.find("declares none") != std::string::npos);
    }
    {
        // Relocalisable d'un cote, placee par son `org` de l'autre : la section
        // fusionnee ne peut pas etre les deux, et il n'y a pas de lecture par
        // defaut a preferer.
        asmb::Object a = secObj("a.fo", {{"code", {1}}}, "a.asm", 2);
        asmb::Object b = secObj("b.fo", {{"code", {2}}}, "b.asm", 5);
        b.sections[0].relocatable = false;
        b.fragments[0].placed = true;
        b.fragments[0].relocSection = -1;
        b.fragments[0].addr = 0x8000;
        b.fragments[0].logical = 0x8000;
        link::Image img = link::build({a, b});
        ok("relocalisable ici, absolue la : refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le refus nomme les deux facons de placer",
           m.find("lets the linker place it") != std::string::npos &&
           m.find("places it with 'org'") != std::string::npos);
    }
    {
        // Le plafond sur la SOMME : chaque unite tient, leur somme non. C'est le
        // refus que l'assembleur ne pouvait pas prononcer, puisqu'il ne voit
        // qu'une unite.
        asmb::Object a = secObj("a.fo", {{"blob", {1, 2}}}, "a.asm", 3);
        asmb::Object b = secObj("b.fo", {{"blob", {3, 4}}}, "b.asm", 5);
        a.sections[0].hasMax = true; a.sections[0].max = 3;
        b.sections[0].hasMax = true; b.sections[0].max = 3;
        link::Image img = link::build({a, b});
        ok("la somme des unites depasse le plafond", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le depassement est chiffre", m.find("0x4 > 0x3") != std::string::npos);
        ok("et le nombre d'unites est dit", m.find("summed over 2 units") != std::string::npos);
        ok("rapporte sur la ligne qui PORTE le plafond",
           !img.errors.empty() && img.errors[0].file == "a.asm" && img.errors[0].line == 3);
    }
    {
        // Deux unites qui se contredisent SUR le plafond : le controle de la
        // somme se tait. Verifier une somme contre un plafond qu'on vient de
        // declarer indecidable serait tirer au sort une des deux lectures, puis
        // rapporter un depassement sur ce tirage.
        asmb::Object a = secObj("a.fo", {{"blob", {1, 2}}}, "a.asm", 3);
        asmb::Object b = secObj("b.fo", {{"blob", {3, 4}}}, "b.asm", 5);
        a.sections[0].hasMax = true; a.sections[0].max = 3;
        link::Image img = link::build({a, b});
        ok("un desaccord, un seul diagnostic", img.errors.size() == 1);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et c'est celui du desaccord, pas celui de la somme",
           m.find("keeps the maximum size") != std::string::npos);
    }
    {
        // Une seule unite qui deborde : le linker se TAIT. L'assembleur l'a deja
        // refusee a l'etape A1, et deux diagnostics pour un seul fait en valent
        // zero.
        asmb::Object a = secObj("a.fo", {{"blob", {1, 2, 3, 4}}}, "a.asm", 3);
        a.sections[0].hasMax = true; a.sections[0].max = 3;
        link::Image img = link::build({a});
        ok("un debordement d'une seule unite n'est pas double", img.ok);
    }

    // --- C1.4 : l'ORG deduit de la fenetre ----------------------------------
    // Le profil et le script sont ANALYSES depuis du texte, et non fabriques a la
    // main : c'est le chemin reel, et un test qui echoue nomme le bon maillon.
    auto prof = [](const char *extra = "") {
        std::string t =
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "WINDOW w2 [0x8000..0xBFFF]\n"
            "BANK base1 SIZE 0x4000 rw STORE 1\n"
            "BANK base2 SIZE 0x4000 rw STORE 2\n"
            "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
            "CONFIG SET ram {\n"
            "  linear    [CODE 0]        { w1 base1  w2 base2 }\n"
            "  ext_w1<b> [CODE %100 | b] { w1 ext<b>          }\n"
            "}\n"
            "SELECT ram = OUT 0x7F00, CODE\n";
        t += extra;
        profile::Profile p = profile::parse(t, "m.prof");
        if (!p.ok && !p.errors.empty())
            printf("    PROFIL FAUTIF : %s\n", p.errors[0].message.c_str());
        return p;
    };
    auto scr = [](const std::string &text) { return script::parse(text, "game.ld"); };

    {
        // La fenetre donne l'adresse logique, la configuration donne la banque,
        // et le profil dit sous quel numero cette banque se range. Aucun `org`
        // dans la source : c'est le renversement du §12.2.
        link::Image img = link::build({secObj("a.fo", {{"main", {1, 2, 3}}})},
                                      scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main } } }"),
                                      prof());
        ok("le placement calcule est vert", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("la fenetre donne l'adresse", img.blocks.size() == 1 && img.blocks[0].addr == 0x4000);
        ok("et la configuration donne la banque", img.blocks.size() == 1 && img.blocks[0].bank == 1);
        ok("la banque ecrite est celle-la", img.banksWritten == std::vector<int>{1});
    }
    {
        // Une banque ETENDUE vue dans la meme fenetre : meme adresse logique,
        // rangement different. C'est ce qu'aucune derivation par l'adresse ne
        // saurait faire, et toute la raison de l'etage.
        link::Image img = link::build({secObj("a.fo", {{"audio", {9}}})},
                                      scr("MEMORY_MAP { CONFIG ext_w1<1> { w1 { SECTION audio } } }"),
                                      prof());
        ok("une banque etendue se place", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("l'adresse logique reste celle de la fenetre",
           img.blocks.size() == 1 && img.blocks[0].addr == 0x4000);
        ok("le rangement vient du profil, non de l'adresse",
           img.blocks.size() == 1 && img.blocks[0].bank == 5);
    }
    {
        // LE CONTROLE QUI COMPTE : deplacer la section dans le SCRIPT SEUL change
        // sa banque de rangement, et pas une adresse logique.
        asmb::Object o = secObj("a.fo", {{"audio", {9}}});
        asmb::Symbol sy;
        sy.name = "audio_init"; sy.frag = 0; sy.offset = 0; sy.section = "audio";
        o.symbolTable.push_back(sy);
        link::Image a = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<1> { w1 { SECTION audio } } }"), prof());
        link::Image b = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<2> { w1 { SECTION audio } } }"), prof());
        ok("les deux placements sont verts", a.ok && b.ok);
        ok("la banque change", a.blocks.size() == 1 && b.blocks.size() == 1 &&
                              a.blocks[0].bank == 5 && b.blocks[0].bank == 6);
        ok("et pas une adresse logique",
           a.symbolTable.size() == 1 && b.symbolTable.size() == 1 &&
           a.symbolTable[0].value == 0x4000 && b.symbolTable[0].value == 0x4000);
        ok("la table des symboles porte le rangement decide",
           a.symbolTable[0].bank == 5 && b.symbolTable[0].bank == 6);
    }
    {
        // Deux sections dans la meme fenetre se suivent, dans l'ordre du script.
        link::Image img = link::build({secObj("a.fo", {{"one", {1, 2}}, {"two", {3}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION two  SECTION one } } }"), prof());
        ok("deux sections dans une fenetre se suivent", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        // L'ordre du SCRIPT, non celui de la declaration : `two` d'abord.
        okBytes("dans l'ordre du script, non celui de la source", img.bin, {3, 1, 2});
    }
    {
        // Une section que le script ne nomme pas suit le placement DERIVABLE du
        // §9. Le cas simple ne paie rien, meme quand un script existe.
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}, {"libre", {7}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main } } }"), prof());
        ok("une section hors du script est quand meme placee", img.ok);
        bool seen = false;
        for (const link::Block &b : img.blocks)
            if (b.bytes == std::vector<uint8_t>{7} && b.addr == 0) seen = true;
        ok("et elle suit le placement derivable", seen);
    }
    {
        // Sans script ni profil : EXACTEMENT le placement de l'etage B. C'est
        // l'engagement du §12.1, tenu par une valeur et non par une intention.
        link::Image with = link::build({secObj("a.fo", {{"main", {1, 2, 3}}})},
                                       script::Script(), profile::Profile());
        link::Image without = link::build({secObj("a.fo", {{"main", {1, 2, 3}}})});
        ok("un script vide et un profil vide sont des valeurs licites", with.ok && without.ok);
        ok("et elles donnent le placement de l'etage B",
           with.bin == without.bin && with.blocks.size() == without.blocks.size() &&
           !with.blocks.empty() && with.blocks[0].addr == 0 && with.blocks[0].bank == 0);
    }

    // --- Ce que seul cet endroit peut refuser --------------------------------
    {
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main } } }"), profile::Profile());
        ok("un script qui place sans profil est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus dit d'ou viennent l'adresse et la banque",
           m.find("--target or -P") != std::string::npos);
    }
    {
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG pas_un_etat { w1 { SECTION main } } }"), prof());
        ok("une configuration que le profil ne declare pas est refusee", !img.ok);
        ok("et le refus la nomme",
           !img.errors.empty() &&
           img.errors[0].message.find("'pas_un_etat'") != std::string::npos);
    }
    {
        // `linear` ne dit rien de `w1`... si, justement. Prenons une fenetre dont
        // l'etat ne parle pas : `ext_w1<b>` ne concerne que `w1`.
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG ext_w1<1> { w2 { SECTION main } } }"), prof());
        ok("placer dans une fenetre dont l'etat ne parle pas est refuse", !img.ok);
        ok("et le refus le dit ainsi",
           !img.errors.empty() &&
           img.errors[0].message.find("says nothing about window 'w2'") != std::string::npos);
    }
    {
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG linear { pas_une_fenetre { SECTION main } } }"), prof());
        ok("une fenetre que le profil ne declare pas est refusee", !img.ok);
        ok("et le refus la nomme",
           !img.errors.empty() &&
           img.errors[0].message.find("not a WINDOW") != std::string::npos);
    }
    {
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION jamais_declaree } } }"), prof());
        ok("placer une section qu'aucun objet ne porte est refuse", !img.ok);
        ok("et le refus la nomme",
           !img.errors.empty() &&
           img.errors[0].message.find("which no object declares") != std::string::npos);
    }
    {
        // Une section placee par son `org` ET par le script : il n'y a pas de
        // lecture par defaut a preferer, donc refus.
        asmb::Object o = secObj("a.fo", {{"code", {1}}});
        o.sections[0].relocatable = false;
        o.fragments[0].placed = true;
        o.fragments[0].relocSection = -1;
        o.fragments[0].addr = o.fragments[0].logical = 0x8000;
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION code } } }"), prof());
        ok("une section a la fois org-ee et placee est refusee", !img.ok);
        ok("et le refus dit qu'il faut choisir",
           !img.errors.empty() &&
           img.errors[0].message.find("never both") != std::string::npos);
    }
    {
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main }  w2 { SECTION main } } }"), prof());
        ok("une section placee deux fois est refusee", !img.ok);
        ok("et le refus nomme le premier placement",
           !img.errors.empty() &&
           img.errors[0].message.find("placed twice") != std::string::npos);
    }
    {
        // Le depassement est CHIFFRE : pas « ca ne rentre pas », mais de combien
        // et dans quelle banque.
        asmb::Object o = secObj("a.fo", {{"gros", {1}}});
        o.fragments[0].bytes.assign(0x4001, 0xAA);
        o.fragments[0].prov.assign(0x4001, 1);
        o.sections[0].size = 0x4001;
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION gros } } }"), prof());
        ok("une section qui deborde sa banque est refusee", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le depassement est chiffre",
           m.find("overflows bank 'base1'") != std::string::npos &&
           m.find("by 0x1 bytes") != std::string::npos);
    }
    {
        // Un nom d'etat que deux axes portent : le refus nomme la forme qualifiee
        // plutot que d'en choisir un.
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG on { w1 { SECTION main } } }"),
            prof("CONFIG SET rom_a OVER ram { on { w1 base1 } }\n"
                 "SELECT rom_a = OUT 0, MASK 1, CODE\n"
                 "CONFIG SET rom_b OVER ram { on { w2 base2 } }\n"
                 "SELECT rom_b = OUT 0, MASK 2, CODE\n"));
        ok("un nom d'etat ambigu est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus demande de nommer l'axe",
           m.find("name the axis") != std::string::npos);
    }
    {
        // Une section "uninit" occupe la place sans emettre un octet : c'est ce
        // que l'etape A2 a livre, et le placement calcule ne le defait pas.
        asmb::Object o = secObj("a.fo", {{"vide", {0, 0, 0}}, {"apres", {7}}});
        o.sections[0].kind = "UNINIT";
        o.fragments[0].prov.assign(3, 0);   // reserve, jamais ecrit
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION vide  SECTION apres } } }"), prof());
        ok("une section uninit se place", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        bool found = false;
        for (const link::Block &b : img.blocks)
            if (b.addr == 0x4003 && b.bytes == std::vector<uint8_t>{7}) found = true;
        ok("elle occupe la place sans emettre un octet", found);
    }

    // --- C1.5 : le chevauchement inter-sections, et le mou -------------------
    {
        // Deux configurations qui donnent LA MEME banque a LA MEME fenetre : les
        // deux sections atterrissent aux memes octets. C'est le refus que seul un
        // placement calcule peut prononcer — aucune des deux lignes du script
        // n'est fautive en elle-meme, c'est leur conjonction qui l'est.
        link::Image img = link::build({secObj("a.fo", {{"un", {1}}, {"deux", {2}}})},
            scr("MEMORY_MAP { CONFIG linear   { w1 { SECTION un   } }\n"
                "             CONFIG ext_high { w1 { SECTION deux } } }"),
            prof("CONFIG SET ram2 { ext_high [CODE 1] { w1 base1 } }\n"
                 "SELECT ram2 = OUT 0, CODE\n"));
        ok("deux blocs sur la meme banque sont refuses", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le refus nomme la plage et la banque",
           m.find("overlap at &4000-&7FFF") != std::string::npos &&
           m.find("in bank 'base1'") != std::string::npos);
    }
    {
        // DEUX GRILLES SUPERPOSEES. Les banques diffèrent, les adresses non : sur
        // une machine a mapper, une grille de 8 K redecoupe une page de 16 K, et
        // les deux sont actives ensemble. Le refus est CALCULE — aucun `SHADOWS`
        // n'a ete ecrit, donc il n'y avait aucune occasion de l'ecrire faux.
        //
        // A l'interieur d'un etat la carte est fixe, ce qui distingue ce controle
        // de la co-visibilite entre etats, qui est l'affaire de C2.
        profile::Profile mapper = profile::parse(
            "WINDOW page1 [0x4000..0x7FFF]\n"
            "WINDOW m0 [0x4000..0x5FFF]\n"
            "BANK big SIZE 0x4000 ro STORE 0\n"
            "BANK seg SIZE 0x2000 ro STORE 1\n"
            "CONFIG SET slot { s [CODE 0] { page1 big  m0 seg } }\n"
            "SELECT slot = POKE 0x6000, CODE\n", "m.prof");
        link::Image img = link::build({secObj("a.fo", {{"gros", {1}}, {"petit", {2}}})},
            scr("MEMORY_MAP { CONFIG s { page1 { SECTION gros }\n"
                "                        m0    { SECTION petit } } }"), mapper);
        ok("deux grilles superposees sont refusees", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le refus nomme les deux fenetres et la plage",
           m.find("windows 'page1' and 'm0'") != std::string::npos &&
           m.find("&4000-&5FFF") != std::string::npos);
        ok("et les deux banques qui y sont visibles ensemble",
           m.find("'big' and 'seg' are both visible") != std::string::npos);
    }
    {
        // Un `org` d'un cote, un placement calcule de l'autre : le heurt n'est
        // visible que d'ici, et il nomme les deux facons de placer.
        asmb::Object o = secObj("a.fo", {{"calculee", {1, 2, 3}}, {"orgee", {9}}});
        o.sections[1].relocatable = false;
        o.fragments[1].placed = true;
        o.fragments[1].relocSection = -1;
        o.fragments[1].addr = o.fragments[1].logical = 0x4001;
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION calculee } } }"), prof());
        ok("un org qui tombe dans une section placee est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus nomme les deux facons de placer",
           m.find("placed by its 'org'") != std::string::npos &&
           m.find("section 'calculee'") != std::string::npos);
        ok("un seul diagnostic : l'emplacement refuse se tait ensuite",
           img.errors.size() == 1 && img.warnings.empty());
    }
    {
        // LE MOU, chiffre. Ni erreur ni avertissement : meme canal que PRINT,
        // parce qu'un mou n'est pas un defaut.
        link::Image img = link::build({secObj("a.fo", {{"main", {1, 2, 3}}})},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main } } }"), prof());
        ok("le placement est vert", img.ok && img.errors.empty() && img.warnings.empty());
        ok("et le mou est dit", img.prints.size() == 1);
        const std::string m = img.prints.empty() ? std::string() : img.prints[0].message;
        ok("chiffre, situe, et nomme",
           m.find("0x3FFD bytes unused") != std::string::npos &&
           m.find("at &4003") != std::string::npos &&
           m.find("in bank 'base1'") != std::string::npos);
    }
    {
        // Une banque remplie EXACTEMENT ne dit rien : un mou nul n'est pas une
        // information, et le dire quand meme ferait du bruit a chaque build.
        asmb::Object o = secObj("a.fo", {{"pleine", {1}}});
        o.fragments[0].bytes.assign(0x4000, 0xAA);
        o.fragments[0].prov.assign(0x4000, 1);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION pleine } } }"), prof());
        ok("une banque pleine se place", img.ok);
        ok("et ne dit rien de son mou", img.prints.empty());
    }
    {
        // Sans script, aucun mou n'est dit : la banque d'une source qui n'ecrit
        // pas de carte n'a pas de budget.
        link::Image img = link::build({obj1(frag(0x8000, {1, 2, 3}))});
        ok("le cas simple ne dit rien du mou", img.ok && img.prints.empty());
    }

    // --- C1.6 : OFFSET / SIZE, un decoupage DANS une banque ------------------
    {
        // Deux blocs de 8 K dans une banque de 16 K. Ce n'est PAS une banque de
        // 8 K : les deux moities se rangent au meme endroit.
        link::Image img = link::build(
            {secObj("a.fo", {{"bas", {1, 2}}, {"haut", {3}}})},
            scr("MEMORY_MAP { CONFIG linear {\n"
                "  w1 [OFFSET 0x0000, SIZE 0x2000] { SECTION bas  }\n"
                "  w1 [OFFSET 0x2000, SIZE 0x2000] { SECTION haut }\n"
                "} }"), prof());
        ok("deux blocs cohabitent dans une banque", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("le premier est base sur la fenetre",
           img.blocks.size() == 2 && img.blocks[0].addr == 0x4000);
        ok("le second sur la fenetre plus son offset",
           img.blocks.size() == 2 && img.blocks[1].addr == 0x6000);
        ok("et les deux moities sont dans LA MEME banque — ce n'est pas une banque de 8 K",
           img.blocks.size() == 2 && img.blocks[0].bank == 1 && img.blocks[1].bank == 1);
        ok("chaque bloc dit son propre mou", img.prints.size() == 2);
    }
    {
        // Plusieurs sections dans un meme bloc s'y concatenent, dans l'ordre du
        // script, et le mou est celui du bloc et non celui de la banque.
        link::Image img = link::build(
            {secObj("a.fo", {{"a", {1}}, {"b", {2}}})},
            scr("MEMORY_MAP { CONFIG linear {\n"
                "  w1 [OFFSET 0x0000, SIZE 0x100] { SECTION b  SECTION a }\n"
                "} }"), prof());
        ok("deux sections dans un bloc", img.ok);
        okBytes("concatenees dans l'ordre du script", img.bin, {2, 1});
        const std::string m = img.prints.empty() ? std::string() : img.prints[0].message;
        ok("le mou est celui du BLOC, non celui de la banque",
           m.find("0xFE bytes unused") != std::string::npos);
    }
    {
        link::Image img = link::build(
            {secObj("a.fo", {{"trop", {1, 2, 3}}})},
            scr("MEMORY_MAP { CONFIG linear {\n"
                "  w1 [OFFSET 0x0000, SIZE 0x2] { SECTION trop }\n"
                "} }"), prof());
        ok("un debordement du SIZE declare est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et il est chiffre", m.find("by 0x1 bytes") != std::string::npos);
    }
    {
        link::Image img = link::build(
            {secObj("a.fo", {{"a", {1}}, {"b", {2}}})},
            scr("MEMORY_MAP { CONFIG linear {\n"
                "  w1 [OFFSET 0x0000, SIZE 0x2000] { SECTION a }\n"
                "  w1 [OFFSET 0x1000, SIZE 0x2000] { SECTION b }\n"
                "} }"), prof());
        ok("deux blocs qui se recouvrent sont refuses", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("le refus nomme les deux decoupages et la plage",
           m.find("[OFFSET 0x1000, SIZE 0x2000]") != std::string::npos &&
           m.find("[OFFSET 0x0, SIZE 0x2000]") != std::string::npos &&
           m.find("&5000-&5FFF") != std::string::npos);
    }
    {
        link::Image img = link::build(
            {secObj("a.fo", {{"a", {1}}})},
            scr("MEMORY_MAP { CONFIG linear {\n"
                "  w1 [OFFSET 0x3000, SIZE 0x2000] { SECTION a }\n"
                "} }"), prof());
        ok("un decoupage qui sort de sa banque est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus donne la place reelle",
           m.find("does not fit in bank 'base1'") != std::string::npos &&
           m.find("0x4000 bytes") != std::string::npos);
    }

    // --- C1.7 : les symboles de commutation, et bankof() --------------------
    {
        // Les trois valeurs du §12.3, au chiffre pres. Elles se CALCULENT :
        // `%11000000 | (PAGE << 3) | CODE`, avec CODE = %100 | 1 pour
        // `ext_w1<1>` et PAGE = 0 sur une machine a une seule page etendue.
        auto pal = [] {
            return profile::parse(
                "WINDOW w1 [0x4000..0x7FFF]\n"
                "BANK base1 SIZE 0x4000 rw STORE 1\n"
                "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
                "CONFIG SET ram {\n"
                "  linear    [CODE %000]     { w1 base1  }\n"
                "  ext_w1<b> [CODE %100 | b] { w1 ext<b> }\n"
                "}\n"
                "SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE\n", "m.prof");
        };
        // Un objet qui DEMANDE les trois symboles, et que rien n'exporte.
        auto asker = [](std::initializer_list<const char *> names) {
            asmb::Object o = secObj("a.fo", {{"main", {0, 0}}, {"audio", {9}}});
            int off = 0;
            for (const char *n : names) {
                asmb::Reloc r;
                r.frag = 0; r.offset = off; r.kind = asmb::Reloc::Abs16; r.symbol = n;
                o.relocs.push_back(r);
                off = 0;   // toutes au meme endroit : seule leur resolution nous interesse
            }
            return o;
        };
        link::Image img = link::build({asker({"__port_ram_audio"})},
            scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }"), pal());
        ok("un EXTERN sur un symbole du linker se resout, sans qu'aucun objet ne l'exporte",
           img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        okBytes("et il vaut le port du profil", img.bin, {0x00, 0x7F});

        link::Image v = link::build({asker({"__val_ram_audio"})},
            scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }"), pal());
        okBytes("la valeur se calcule : %11000000 | (0 << 3) | %101 = &C5", v.bin, {0xC5, 0x00});

        link::Image l = link::build({asker({"__val_ram_linear"})},
            scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }"), pal());
        okBytes("et la graphie PAR ETAT vaut &C0", l.bin, {0xC0, 0x00});

        link::Image m = link::build({asker({"__val_ram_main"})},
            scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }"), pal());
        okBytes("la graphie PAR SECTION vaut la meme chose", m.bin, {0xC0, 0x00});
    }
    {
        // La valeur est BORNEE AUX BITS DE L'AXE des qu'un masque les nomme :
        // c'est ce qui permet au source d'ecrire `(etat & ~masque) | valeur`
        // sans toucher aux axes voisins. Un symbole qui vaudrait « l'octet »
        // ecraserait les autres axes en silence (D7).
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "BANK rom_lo SIZE 0x4000 ro STORE 8\n"
            "CONFIG SET rom { on [CODE 0] { w0 rom_lo } }\n"
            "SELECT rom = OUT 0x7F00, MASK %00000100, %11111111\n", "m.prof");
        asmb::Object o = secObj("a.fo", {{"boot", {0, 0}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__val_rom_boot";
        o.relocs.push_back(r);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG on { w0 { SECTION boot } } }"), pr);
        ok("l'axe masque se resout", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        // `bin` ne couvre que les 64 K de base ; hors d'elles c'est le BLOC
        // qu'on interroge, et c'est plus juste : un octet range en banque 8
        // n'a rien a faire dans un binaire plat.
        okBytes("et la valeur est bornee au masque, non l'octet entier",
                img.blocks.empty() ? std::vector<uint8_t>() : img.blocks[0].bytes, {0x04, 0x00});
        asmb::Object k = secObj("a.fo", {{"boot", {0, 0}}});
        asmb::Reloc r2;
        r2.frag = 0; r2.offset = 0; r2.kind = asmb::Reloc::Abs16; r2.symbol = "__mask_rom";
        k.relocs.push_back(r2);
        link::Image mi = link::build({k},
            scr("MEMORY_MAP { CONFIG on { w0 { SECTION boot } } }"), pr);
        okBytes("et le masque est offert tel quel",
                mi.blocks.empty() ? std::vector<uint8_t>() : mi.blocks[0].bytes, {0x04, 0x00});
    }
    {
        // LE PORT PEUT ETRE FONCTION DE LA BANQUE (§13.1) : sur une machine assez
        // grande, une partie du numero est dans l'ADRESSE du port. Le langage
        // l'exprime, le profil livre ne l'emploie pas, et ce profil de test
        // l'exerce — ce qui est le seul moyen de savoir qu'il l'exprime.
        profile::Profile pr = profile::parse(
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK big0..big3 SIZE 0x4000 rw STORE 0..3\n"
            "BANK big4 SIZE 0x4000 rw STORE 4 PAGE 2\n"
            "CONFIG SET pg { p<n> [CODE n] { w1 big<n> } }\n"
            "SELECT pg = OUT 0x7F00 - (PAGE << 8), CODE\n", "m.prof");
        asmb::Object o = secObj("a.fo", {{"loin", {0, 0}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__port_pg_loin";
        o.relocs.push_back(r);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG p<4> { w1 { SECTION loin } } }"), pr);
        ok("un port fonction de la banque se calcule", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        okBytes("&7F00 - (2 << 8) = &7D00",
                img.blocks.empty() ? std::vector<uint8_t>() : img.blocks[0].bytes, {0x00, 0x7D});
    }
    {
        // LA GRAPHIE DEUX-REGISTRES TRAVERSE LA COUTURE, et c'est la seule.
        // `__port_` est une adresse sur SEIZE bits : `ld b, __port_...` sortait
        // un octet nul en silence, et `>> 8` comme `|` sont refuses par
        // l'assembleur sur une valeur relocalisable — ils exigent un nombre. Il
        // reste `high()` / `low()`, qui posent une relocalisation, et le linker
        // doit les honorer sur un symbole que LUI SEUL offre.
        profile::Profile pr = profile::parse(
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
            "CONFIG SET ram { ext_w1<b> [CODE %100 | b] { w1 ext<b> } }\n"
            "SELECT ram = OUT 0x7F00, %11000000 | CODE\n", "m.prof");
        asmb::Object o = secObj("a.fo", {{"gfx0", {0, 0}}});
        asmb::Reloc hi;
        hi.frag = 0; hi.offset = 0; hi.kind = asmb::Reloc::High8; hi.symbol = "__port_ram_gfx0";
        asmb::Reloc lo;
        lo.frag = 0; lo.offset = 1; lo.kind = asmb::Reloc::Low8; lo.symbol = "__val_ram_gfx0";
        o.relocs.push_back(hi);
        o.relocs.push_back(lo);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<0> { w1 { SECTION gfx0 } } }"), pr);
        ok("high()/low() se resolvent sur un symbole du linker", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        okBytes("high(&7F00) = &7F, low(&C4) = &C4",
                img.blocks.empty() ? std::vector<uint8_t>() : img.blocks[0].bytes, {0x7F, 0xC4});
    }
    {
        // Un etat PARAMETRIQUE nomme deux fois avec deux arguments ne designe pas
        // une seule chose : la graphie par etat n'est alors PAS offerte, et celle
        // par section reste la bonne. C'est exactement le tableau du §12.3, ou
        // `__val_ram_music_lz` et `__val_ram_audio` coexistent.
        profile::Profile pr = profile::parse(
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
            "CONFIG SET ram { ext_w1<b> [CODE %100 | b] { w1 ext<b> } }\n"
            "SELECT ram = OUT 0x7F00, %11000000 | CODE\n", "m.prof");
        asmb::Object o = secObj("a.fo", {{"un", {0, 0}}, {"deux", {0, 0}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__val_ram_ext_w1";
        o.relocs.push_back(r);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<0> { w1 { SECTION un   } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION deux } } }"), pr);
        ok("une graphie par etat ambigue n'est pas offerte", !img.ok);
        const std::string msg = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et l'EXTERN non resolu le dit",
           msg.find("unresolved EXTERN symbol '__val_ram_ext_w1'") != std::string::npos);
        asmb::Object k = secObj("a.fo", {{"un", {0, 0}}, {"deux", {0, 0}}});
        asmb::Reloc r2;
        r2.frag = 0; r2.offset = 0; r2.kind = asmb::Reloc::Abs16; r2.symbol = "__val_ram_deux";
        k.relocs.push_back(r2);
        link::Image ok2 = link::build({k},
            scr("MEMORY_MAP { CONFIG ext_w1<0> { w1 { SECTION un   } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION deux } } }"), pr);
        // La relocalisation est PORTEE par `un`, qui vit en banque 4 ; c'est la
        // valeur de `deux` qu'elle y ecrit.
        std::vector<uint8_t> got;
        for (const link::Block &b : ok2.blocks) if (b.bank == 4) got = b.bytes;
        okBytes("la graphie par SECTION, elle, est sans ambiguite", got, {0xC5, 0x00});
    }
    {
        // `bankof(x)` : la relocalisation `BankOf` vaut l'EMPLACEMENT DE
        // RANGEMENT de la section visee, et non un octet de son adresse.
        asmb::Object o = secObj("a.fo", {{"main", {0}}, {"audio", {9}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::BankOf; r.section = 1;
        o.relocs.push_back(r);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }"), prof());
        ok("bankof se resout", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("et vaut le STORE de la banque que la configuration donne",
           !img.blocks.empty() && img.blocks[0].bytes == std::vector<uint8_t>{5});
    }
    {
        // Sans script, `bankof` vaut la derivation historique par l'adresse
        // (ADR 0005) : le cas simple ne paie rien, ici non plus.
        asmb::Object o = secObj("a.fo", {{"main", {0}}});
        o.sections[0].relocatable = false;
        o.fragments[0].placed = true;
        o.fragments[0].relocSection = -1;
        o.fragments[0].addr = o.fragments[0].logical = 0x8000;
        asmb::Object t = secObj("b.fo", {{"cible", {7}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::BankOf; r.section = 0;
        o.relocs.push_back(r);
        link::Image img = link::build({o, t});
        ok("sans script, bankof suit l'adresse", img.ok);
    }

    // --- C1.8 : __off_, __romnum_, et les deux ecritures d'un axe -----------
    {
        // Un axe qui demande DEUX ecritures : la premiere dit « cette banque-la
        // apparait », la seconde dit LAQUELLE. C'est le §12.3, ou `__romnum_` est
        // « la seconde ecriture d'un SELECT qui en compte deux ».
        //
        // Et `rom_hi<n>` n'est pas `ext<b>` : la premiere designe UNE banque
        // declaree parametriquement, dont le parametre est un numero que le
        // materiel lui donne ; la seconde choisit parmi des banques reellement
        // declarees. Les deux formes coexistent sans un mot de vocabulaire de
        // plus, parce que la resolution cherche la banque concatenee d'abord.
        profile::Profile pr = profile::parse(
            "WINDOW w3 [0xC000..0xFFFF]\n"
            "BANK rom_hi<n> SIZE 0x4000 ro STORE 9\n"
            "CONFIG SET rom { on<n> [CODE 0] { w3 rom_hi<n> } }\n"
            "SELECT rom = OUT 0x7F00, MASK %00001000, CODE << 3\n"
            "             OUT 0xDF00, MASK %11111111, PAGE\n", "m.prof");
        auto ask = [&](const char *sym) {
            asmb::Object o = secObj("a.fo", {{"menu", {0, 0}}});
            asmb::Reloc r;
            r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = sym;
            o.relocs.push_back(r);
            link::Image img = link::build({o},
                scr("MEMORY_MAP { CONFIG rom.on<15> { w3 { SECTION menu } } }"), pr);
            if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
            std::vector<uint8_t> got;
            for (const link::Block &b : img.blocks) if (b.bank == 9) got = b.bytes;
            return got;
        };
        okBytes("la premiere ecriture donne le port", ask("__port_rom_menu"), {0x00, 0x7F});
        okBytes("et la valeur, bornee au bit de l'axe", ask("__val_rom_menu"), {0x00, 0x00});
        okBytes("la seconde donne son port", ask("__port2_rom_menu"), {0x00, 0xDF});
        okBytes("et le NUMERO, que PAGE vaut ici", ask("__romnum_rom_menu"), {0x0F, 0x00});
    }
    {
        // `__off_<section>` : son offset DANS sa banque, pour un loader ou une
        // recopie (§12.3). Celui-la depend du PLACEMENT, donc il ne peut pas etre
        // une constante calculee avant d'assembler — c'est la seule difference
        // de nature entre les deux familles.
        asmb::Object o = secObj("a.fo", {{"un", {1, 2, 3}}, {"deux", {0, 0}}});
        asmb::Reloc r;
        r.frag = 1; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__off_deux";
        o.relocs.push_back(r);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION un  SECTION deux } } }"), prof());
        ok("__off_ se resout", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        // `deux` suit `un` : offset 3 dans la banque, et non l'adresse &4003.
        std::vector<uint8_t> got;
        for (const link::Block &b : img.blocks) if (b.addr == 0x4003) got = b.bytes;
        okBytes("et vaut l'offset dans la banque, non l'adresse", got, {0x03, 0x00});
    }
    {
        // Un qualificatif que rien ne consomme est REFUSE plutot qu'ignore :
        // l'auteur croirait avoir dit quelque chose. Le nombre d'un etat
        // parametrique se dit dans son argument, comme partout ailleurs.
        link::Image img = link::build({secObj("a.fo", {{"main", {1}}})},
            scr("MEMORY_MAP { CONFIG linear, ROM 15 { w1 { SECTION main } } }"), prof());
        ok("un qualificatif inconsomme est refuse", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus donne la forme juste",
           m.find("is not consumed") != std::string::npos &&
           m.find("linear<n>") != std::string::npos);
    }

    {
        // La PLACE DEMANDEE compte, et pas seulement les octets poses. Une
        // section "uninit" n'emet rien : sans ce controle son etendue serait
        // nulle, le linker ne lui donnerait pas d'adresse, et son label vaudrait
        // zero — ce qui est exactement ce que l'exemple d'acceptation a montre.
        asmb::Object o = secObj("a.fo", {{"vars", {}}, {"apres", {7}}});
        o.sections[0].kind = "UNINIT";
        o.sections[0].size = 0x100;      // `ds 0x100` : reserve, jamais emis
        asmb::Symbol sy;
        sy.name = "vars"; sy.frag = 0; sy.offset = 0; sy.section = "vars";
        o.symbolTable.push_back(sy);
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION vars  SECTION apres } } }"), prof());
        ok("une section uninit sans un octet est quand meme placee", img.ok);
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("son label vaut l'adresse de sa fenetre, non zero",
           img.symbolTable.size() == 1 && img.symbolTable[0].value == 0x4000);
        bool after = false;
        for (const link::Block &b : img.blocks)
            if (b.addr == 0x4100 && b.bytes == std::vector<uint8_t>{7}) after = true;
        ok("et la section suivante commence APRES la place reservee", after);
    }

    // --- Le point d'entree passe par la base de sa section -------------------
    {
        // Un `run` qui nomme un label d'une section RELOCALISABLE. Le defaut
        // existait depuis l'etage B, ou il ne se voyait pas : les sections
        // relocalisables s'y posaient a la base zero pour une source sans octet
        // absolu, si bien qu'un offset passait pour une adresse. Le placement
        // calcule le rend certain.
        asmb::Object o = secObj("a.fo", {{"main", {1, 2, 3}}});
        asmb::Symbol sy;
        sy.name = "start"; sy.frag = 0; sy.offset = 1; sy.section = "main";
        o.symbolTable.push_back(sy);
        o.symbols["start"] = 1;          // l'OFFSET, tel que l'assembleur le sait
        o.entry.has = true;
        o.entry.name = "start";
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG linear { w1 { SECTION main } } }"), prof());
        ok("le run est resolu", img.ok);
        ok("il vaut la BASE de sa section plus son offset, non son offset seul",
           img.runAddress == 0x4001);
    }
    {
        // Le meme, sans script : la base vient du placement derivable, et le run
        // la suit. C'est le cas ou le defaut se cachait.
        asmb::Object o = secObj("a.fo", {{"main", {1, 2, 3}}});
        asmb::Symbol sy;
        sy.name = "start"; sy.frag = 0; sy.offset = 2; sy.section = "main";
        o.symbolTable.push_back(sy);
        o.symbols["start"] = 2;
        o.entry.has = true;
        o.entry.name = "start";
        asmb::Object abs = obj1(frag(0x8000, {0xAA}), "b.asm", 1);
        abs.name = "b.fo";
        link::Image img = link::build({abs, o});
        ok("sans script non plus, le run ne perd pas sa base",
           img.ok && img.runAddress == 0x8003);
    }

    // --- Rien a lier --------------------------------------------------------
    {
        link::Image img = link::build({});
        ok("aucun objet donne une image vide et valide",
           img.ok && img.bin.empty() && img.blocks.empty() && img.banksWritten.empty());
    }

    // --- P1 : les cles du profil seul ---------------------------------------
    // `switchSymbols` est appelee AVANT d'assembler (asm_main.cpp), donc elle ne
    // peut pas savoir ce que le source nomme : ses cles doivent se deriver du
    // PROFIL seul. Sans script, elle ne rendait rien — sa boucle de tete etait
    // celle des blocs du script.
    {
        auto pal = [] {
            return profile::parse(
                "WINDOW w1 [0x4000..0x7FFF]\n"
                "BANK base1 SIZE 0x4000 rw STORE 1\n"
                "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
                "CONFIG SET ram {\n"
                "  linear    [CODE %000]     { w1 base1  }\n"
                "  all_ext   [CODE %010]     { w1 ext1   }\n"
                "  ext_w1<b> [CODE %100 | b] { w1 ext<b> }\n"
                "}\n"
                "SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE\n", "m.prof");
        };
        const profile::Profile pr = pal();
        const script::Script none;
        const std::map<std::string, int64_t> sy = link::switchSymbols(none, pr);
        auto val = [&](const char *n) -> int64_t {
            auto it = sy.find(n);
            return it == sy.end() ? -1 : it->second;
        };

        ok("sans script, un etat SANS parametre garde la graphie d'aujourd'hui",
           val("__val_ram_linear") == 0xC0);
        ok("et son port sort aussi", val("__port_ram_linear") == 0x7F00);
        ok("un etat PARAMETRIQUE porte son argument dans le nom",
           val("__val_ram_ext_w1_1") == 0xC5);
        ok("les valeurs que les banques DECLAREES bornent sont toutes enumerees",
           val("__val_ram_ext_w1_0") == 0xC4 && val("__val_ram_ext_w1_2") == 0xC6 &&
           val("__val_ram_ext_w1_3") == 0xC7);
        ok("une valeur qu'aucune banque declaree ne porte n'est pas offerte",
           val("__val_ram_ext_w1_4") == -1);
        // Le motif meme du chantier : `ext1` est vue en w1 par DEUX cartes, et
        // une cle par banque vaudrait deux choses. Une cle par ETAT en vaut une.
        ok("les deux cartes qui voient ext1 sont DISTINGUEES, pas fusionnees",
           val("__val_ram_all_ext") == 0xC2 && val("__val_ram_ext_w1_1") == 0xC5);
        ok("l'etat parametrique sans son argument n'est pas offert : il ne designe rien",
           val("__val_ram_ext_w1") == -1);
    }
    {
        // L'invariant de C1.7 ne se perd pas en changeant de source de cles : la
        // valeur reste BORNEE AUX BITS DE L'AXE (D7), et le masque sort aussi.
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "BANK rom_lo SIZE 0x4000 ro STORE 8\n"
            "CONFIG SET rom { on [CODE 0] { w0 rom_lo } }\n"
            "SELECT rom = OUT 0x7F00, MASK %00000100, %11111111\n", "m.prof");
        const script::Script none;
        const std::map<std::string, int64_t> sy = link::switchSymbols(none, pr);
        auto val = [&](const char *n) -> int64_t {
            auto it = sy.find(n);
            return it == sy.end() ? -1 : it->second;
        };
        ok("sans script, la valeur reste bornee aux bits de l'axe", val("__val_rom_on") == 0x04);
        ok("et le masque de l'axe sort", val("__mask_rom") == 0x04);
    }
    {
        // Le contrat de non-regression : AVEC un script, rien ne change. Les
        // cles par section restent celles du script, et les cles par etat
        // valent la meme chose par les deux chemins — donc la regle de retrait
        // ne les efface pas.
        profile::Profile pr = profile::parse(
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK base1 SIZE 0x4000 rw STORE 1\n"
            "BANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
            "CONFIG SET ram {\n"
            "  linear    [CODE %000]     { w1 base1  }\n"
            "  ext_w1<b> [CODE %100 | b] { w1 ext<b> }\n"
            "}\n"
            "SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE\n", "m.prof");
        script::Script sc = scr("MEMORY_MAP { CONFIG linear    { w1 { SECTION main  } }\n"
                                "             CONFIG ext_w1<1> { w1 { SECTION audio } } }");
        const std::map<std::string, int64_t> sy = link::switchSymbols(sc, pr);
        auto val = [&](const char *n) -> int64_t {
            auto it = sy.find(n);
            return it == sy.end() ? -1 : it->second;
        };
        ok("avec un script, la cle PAR SECTION est intacte",
           val("__val_ram_main") == 0xC0 && val("__val_ram_audio") == 0xC5);
        ok("et la cle PAR ETAT n'est pas effacee par la seconde offre du profil",
           val("__val_ram_linear") == 0xC0);
        ok("la cle du profil coexiste avec celles du script",
           val("__val_ram_ext_w1_1") == 0xC5);
    }

    {
        // LA FUSION EST A SENS UNIQUE. La graphie « par etat » est offerte par
        // les deux chemins, et ils ne repondent pas a la meme question : le
        // script la calcule POUR LA FENETRE qu'il a placee, le profil pour
        // toutes les fenetres de l'etat. Ici les deux fenetres de `both` donnent
        // deux valeurs — le profil doit donc se taire, et NON retirer le symbole
        // que le script offrait en sachant, lui, de quelle fenetre il parlait.
        auto pal = [] {
            return profile::parse(
                "WINDOW w0 [0x0000..0x3FFF]\n"
                "WINDOW w1 [0x4000..0x7FFF]\n"
                "BANK lo SIZE 0x4000 rw PAGE 0 STORE 0\n"
                "BANK hi SIZE 0x4000 rw PAGE 1 STORE 1\n"
                "CONFIG SET ram { both [CODE 0] { w0 lo  w1 hi } }\n"
                "SELECT ram = OUT 0x7F00, (PAGE << 3) | CODE\n", "m.prof");
        };
        auto val = [](const std::map<std::string, int64_t> &m, const char *n) -> int64_t {
            auto it = m.find(n);
            return it == m.end() ? -1 : it->second;
        };
        const profile::Profile pr = pal();
        const script::Script none;
        const std::map<std::string, int64_t> alone = link::switchSymbols(none, pr);
        ok("un etat dont deux fenetres donnent deux valeurs n'est pas offert par le profil seul",
           val(alone, "__val_ram_both") == -1);
        // Le port est le meme par les deux fenetres la ou la valeur differe : la
        // regle de retrait n'effacait que la valeur, et laissait la moitie d'un
        // couple que le §12.3 emploie d'un bloc.
        ok("et son port ne lui survit pas", val(alone, "__port_ram_both") == -1);

        script::Script sc = scr("MEMORY_MAP { CONFIG both { w1 { SECTION s } } }");
        const std::map<std::string, int64_t> sy = link::switchSymbols(sc, pr);
        ok("mais le script, qui sait DE QUELLE FENETRE il parle, l'offre toujours",
           val(sy, "__val_ram_both") == 0x08 && val(sy, "__val_ram_s") == 0x08);
    }

    {
        // Les valeurs du parametre sont une INTERSECTION, non une reunion. Une
        // valeur que l'une des fenetres de l'etat ne sait pas honorer ne designe
        // pas une carte a moitie atteignable : elle n'en designe aucune, et le
        // placement la refuse deja — « the profile declares no such bank ».
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK lo0..lo1 SIZE 0x4000 rw STORE 0..1\n"
            "BANK hi0..hi3 SIZE 0x4000 rw STORE 2..5\n"
            "CONFIG SET x { s<b> [CODE b] { w0 lo<b>  w1 hi<b> } }\n"
            "SELECT x = OUT 0x1234, CODE\n", "m.prof");
        const script::Script none;
        const std::map<std::string, int64_t> sy = link::switchSymbols(none, pr);
        auto val = [&](const char *n) -> int64_t {
            auto it = sy.find(n);
            return it == sy.end() ? -1 : it->second;
        };
        ok("une valeur que TOUTES les fenetres honorent est offerte",
           val("__val_x_s_0") == 0 && val("__val_x_s_1") == 1);
        ok("une valeur qu'une seule fenetre honore ne l'est pas",
           val("__val_x_s_2") == -1 && val("__val_x_s_3") == -1);
        ok("et pas davantage son port : une carte inatteignable n'offre rien",
           val("__port_x_s_2") == -1);
    }
    {
        // UN ETAT QUI NE MAPPE AUCUNE FENETRE EN EST UN QUAND MEME. Le `off`
        // d'un axe de recouvrement est ce qui REND la RAM, et c'est une valeur
        // qu'aucun script ne pourra jamais offrir — il n'y a rien a y placer.
        // Sans elle, le masque de l'axe n'existerait pas non plus.
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "BANK base0 SIZE 0x4000 rw STORE 0\n"
            "BANK rom_lo SIZE 0x4000 ro STORE 8\n"
            "CONFIG SET ram { flat [CODE 0] { w0 base0 } }\n"
            "CONFIG SET rom OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo } }\n"
            "SELECT ram = OUT 0x7F00, %11000000 | CODE\n"
            "SELECT rom = OUT 0x7F00, MASK %00000100, CODE << 2\n", "m.prof");
        const script::Script none;
        const std::map<std::string, int64_t> sy = link::switchSymbols(none, pr);
        auto val = [&](const char *n) -> int64_t {
            auto it = sy.find(n);
            return it == sy.end() ? -1 : it->second;
        };
        ok("l'etat qui REND la RAM est offert, bien qu'il ne place rien",
           val("__val_rom_off") == 0x04 && val("__port_rom_off") == 0x7F00);
        ok("celui qui prend la ROM aussi", val("__val_rom_on") == 0x00);
        ok("et le masque de l'axe existe, ce qui rend le bit touchable seul",
           val("__mask_rom") == 0x04);
    }

    // --- P3 : le linker honore le placement que le SOURCE porte -------------
    // Le placement du source entre dans LA MEME CARTE que celle du script : un
    // seul moteur, donc un seul jeu de diagnostics. Les controles ci-dessous
    // n'exercent pas des mecanismes neufs — ils verifient que les anciens
    // s'appliquent, ce qui est la preuve qu'aucun second moteur n'est ne.
    auto place = [](asmb::Object &o, const char *sec, const char *cfg, const char *win = "") {
        for (asmb::Section &s : o.sections)
            if (s.name == sec) { s.place = cfg; s.placeWindow = win; }
    };
    auto pal3 = [] {
        return profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "WINDOW w2 [0x8000..0xBFFF]\n"
            "BANK base0..base2 SIZE 0x4000 rw STORE 0..2\n"
            "BANK ext0..ext3   SIZE 0x4000 rw STORE 4..7\n"
            "BANK rom_lo       SIZE 0x4000 ro STORE 8\n"
            "CONFIG SET ram {\n"
            "  linear    [CODE %000]     { w0 base0  w1 base1  w2 base2 }\n"
            "  ext_w1<b> [CODE %100 | b] { w1 ext<b> }\n"
            "}\n"
            "CONFIG SET rom OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo } }\n"
            "SELECT ram = OUT 0x7F00, %11000000 | CODE\n"
            "SELECT rom = OUT 0x7F00, MASK %00000100, CODE << 2\n", "m.prof");
    };
    {
        // Le cas en titre : sans un mot de script, la section va dans la banque
        // que sa configuration designe, a l'adresse que la fenetre donne.
        asmb::Object o = secObj("a.fo", {{"gfx1", {0xA1, 0x10}}});
        place(o, "gfx1", "ext_w1<1>");
        link::Image img = link::build({o}, script::Script(), pal3());
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("une section que le SOURCE place est placee, sans script", img.ok);
        ok("dans la banque que la configuration designe, a l'adresse de la fenetre",
           img.ok && img.banksWritten.size() == 1 && img.banksWritten[0] == 5);
        const link::Flat flat = link::flatten(img);
        ok("et les octets y sont", flat.bytes.size() > (5 * 0x4000 + 1) &&
           flat.bytes[5 * 0x4000] == 0xA1 && flat.bytes[5 * 0x4000 + 1] == 0x10);
    }
    {
        // La forme verbeuse designe la fenetre elle-meme.
        asmb::Object o = secObj("a.fo", {{"main", {0xAA}}});
        place(o, "main", "linear", "w2");
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("la forme verbeuse place dans la fenetre nommee",
           img.ok && !img.bin.empty() && img.loadAddress == 0x8000);
    }
    {
        asmb::Object o = secObj("a.fo", {{"gfx1", {1}}});
        place(o, "gfx1", "nulle_part");
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("une configuration que le profil ne declare pas est refusee", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus nomme celles qu'il declare",
           m.find("nulle_part") != std::string::npos &&
           m.find("linear") != std::string::npos && m.find("ext_w1") != std::string::npos);
    }
    {
        // La forme COURTE ne vaut que si la configuration ne mappe qu'une
        // fenetre. `linear` en mappe trois : la section irait ou ?
        asmb::Object o = secObj("a.fo", {{"main", {1}}});
        place(o, "main", "linear");
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("la forme courte sur une config a plusieurs fenetres est refusee", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus nomme les fenetres, et la forme verbeuse",
           m.find("w0") != std::string::npos && m.find("w2") != std::string::npos &&
           m.find("OF") != std::string::npos);
    }
    {
        asmb::Object o = secObj("a.fo", {{"gfx1", {1}}});
        place(o, "gfx1", "ext_w1<1>", "w2");
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("une fenetre que la configuration ne mappe pas est refusee", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus nomme celles qu'elle mappe",
           m.find("w2") != std::string::npos && m.find("w1") != std::string::npos);
    }
    {
        // LE CHEVAUCHEMENT S'APPLIQUE, sans une ligne de plus. C'est le controle
        // qui prouve qu'aucun second moteur de placement n'est ne.
        asmb::Object o = secObj("a.fo", {{"gfx1", std::vector<uint8_t>(0x3000, 1)},
                                         {"gfx2", std::vector<uint8_t>(0x3000, 2)}});
        place(o, "gfx1", "ext_w1<1>");
        place(o, "gfx2", "ext_w1<1>");
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("deux sections que le source place au-dela de la banque sont refusees", !img.ok);
    }
    {
        // Et le MOU est chiffre, par le meme chemin qu'un placement de script.
        asmb::Object o = secObj("a.fo", {{"gfx1", std::vector<uint8_t>(0x2000, 1)}});
        place(o, "gfx1", "ext_w1<1>");
        link::Image img = link::build({o}, script::Script(), pal3());
        bool said = false;
        for (const auto &p : img.prints)
            if (p.message.find("unused") != std::string::npos) said = true;
        ok("le mou de la banque est signale et chiffre", img.ok && said);
    }
    {
        // L'ORDRE DE CONCATENATION est celui de la fusion par nom de C1.0 :
        // les objets dans l'ordre de la ligne de commande, puis l'ordre de
        // declaration dans chacun.
        asmb::Object a = secObj("a.fo", {{"un", {0x11}}});
        asmb::Object b = secObj("b.fo", {{"deux", {0x22}}});
        place(a, "un", "ext_w1<1>");
        place(b, "deux", "ext_w1<1>");
        link::Image img = link::build({a, b}, script::Script(), pal3());
        const link::Flat flat = link::flatten(img);
        ok("deux sections dans la meme fenetre se concatenent dans cet ordre",
           img.ok && flat.bytes.size() > (5 * 0x4000 + 1) &&
           flat.bytes[5 * 0x4000] == 0x11 && flat.bytes[5 * 0x4000 + 1] == 0x22);
    }
    {
        // Le SCRIPT gagne sur le source, mais PAS EN SILENCE. Le piege n'est pas
        // cosmetique : le source commute avec la valeur de SA configuration, qui
        // est la mauvaise des que le script l'a pose ailleurs — et la faute ne se
        // voit ni dans l'un ni dans l'autre fichier pris seul.
        asmb::Object o = secObj("a.fo", {{"gfx1", {0xEE}}});
        place(o, "gfx1", "ext_w1<1>");
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<3> { w1 { SECTION gfx1 } } }"), pal3());
        ok("quand le script place aussi, c'est lui qui decide",
           img.ok && img.banksWritten.size() == 1 && img.banksWritten[0] == 7);
        bool said = false, pointed = false;
        for (const auto &w : img.warnings) {
            if (w.message.find("the script wins") != std::string::npos) said = true;
            if (w.message.find("that wins") != std::string::npos) pointed = true;
        }
        ok("et un avertissement dit que le IN ne s'applique pas", said);
        ok("en montrant le placement qui l'emporte", pointed);
    }
    {
        // Une section que SEUL le source place n'avertit de rien.
        asmb::Object o = secObj("a.fo", {{"gfx1", {1}}});
        place(o, "gfx1", "ext_w1<1>");
        link::Image img = link::build({o}, script::Script(), pal3());
        bool said = false;
        for (const auto &w : img.warnings)
            if (w.message.find("the script wins") != std::string::npos) said = true;
        ok("un placement que rien ne surcharge est silencieux", img.ok && !said);
    }
    {
        // Deux unites qui placent le meme nom se comparent sur l'identite
        // RESOLUE : `ext_w1<1>` et `ram.ext_w1<1>` nomment une seule chose, et le
        // versement les traite deja comme une seule.
        asmb::Object a = secObj("a.fo", {{"gfx1", {0x11}}});
        asmb::Object b = secObj("b.fo", {{"gfx1", {0x22}}});
        place(a, "gfx1", "ext_w1<1>");
        place(b, "gfx1", "ram.ext_w1<1>");
        link::Image img = link::build({a, b}, script::Script(), pal3());
        if (!img.ok && !img.errors.empty()) printf("    %s\n", img.errors[0].message.c_str());
        ok("deux graphies d'une meme configuration ne sont pas un desaccord", img.ok);
        asmb::Object c = secObj("c.fo", {{"gfx1", {0x33}}});
        place(c, "gfx1", "ext_w1<2>");
        link::Image bad = link::build({a, c}, script::Script(), pal3());
        ok("deux configurations differentes en sont un", !bad.ok);
    }
    {
        // Un nom que deux axes portent est qualifie de son axe dans la liste ;
        // les autres restent nus.
        // Deux axes de recouvrement nomment volontiers `on` et `off` chacun :
        // les lister nus ferait revenir le meme mot deux fois sans dire lequel
        // est lequel — c'est le cas du profil livre, qui en porte deux.
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "WINDOW w3 [0xC000..0xFFFF]\n"
            "BANK base0 SIZE 0x4000 rw STORE 0\n"
            "BANK rom_lo SIZE 0x4000 ro STORE 8\n"
            "BANK rom_hi SIZE 0x4000 ro STORE 9\n"
            "CONFIG SET ram { linear [CODE 0] { w0 base0 } }\n"
            "CONFIG SET lower OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo } }\n"
            "CONFIG SET upper OVER ram { off [CODE 1] { }  on [CODE 0] { w3 rom_hi } }\n"
            "SELECT ram = OUT 0x7F00, CODE\n"
            "SELECT lower = OUT 0x7F00, MASK %100, CODE << 2\n"
            "SELECT upper = OUT 0x7F00, MASK %1000, CODE << 3\n", "m.prof");
        asmb::Object o = secObj("a.fo", {{"s", {1}}});
        place(o, "s", "nulle_part");
        link::Image img = link::build({o}, script::Script(), pr);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("la liste des configurations qualifie ce qui serait ambigu",
           m.find("lower.on") != std::string::npos && m.find("upper.on") != std::string::npos);
        ok("et laisse nu ce qui ne l'est pas",
           m.find("ram.linear") == std::string::npos && m.find("linear") != std::string::npos);
    }
    {
        // Les cles PAR SECTION restent script-seul : `switchSymbols` tourne
        // avant d'assembler, et une cle qui existerait au linkage mais pas a
        // l'assemblage vaudrait deux langages pour un nom.
        asmb::Object o = secObj("a.fo", {{"gfx1", {1}}});
        place(o, "gfx1", "ext_w1<1>");
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__val_ram_gfx1";
        o.relocs.push_back(r);
        link::Image img = link::build({o}, script::Script(), pal3());
        ok("un placement porte par le source n'offre PAS de cle par section", !img.ok);
    }

    // --- La relecture de P5 : quatre defauts, epingles ----------------------
    {
        // UN DECOUPAGE `[OFFSET, SIZE]` NE SE REJOINT PAS DE DEHORS. Un bloc de
        // plus, pleine fenetre, chevaucherait le decoupage et ferait prononcer
        // un refus de RECOUVREMENT dont aucune des deux lignes n'est fautive.
        asmb::Object o = secObj("a.fo", {{"a", {1}}, {"b", {1}}});
        place(o, "b", "ext_w1<1>", "w1");
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<1> { w1 [OFFSET 0x0000, SIZE 0x2000]"
                " { SECTION a } } }"), pal3());
        ok("une section source dans une fenetre decoupee par le script est refusee", !img.ok);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("et le refus nomme le decoupage, PAS un recouvrement",
           m.find("carves") != std::string::npos && m.find("place this one in the script") != std::string::npos &&
           m.find("overlap") == std::string::npos);
        ok("en montrant la ligne qui decoupe",
           img.errors.size() > 1 && img.errors[1].message.find("carves window") != std::string::npos);
    }
    {
        // ET C'EST POURQUOI LE REFUS EST LA BONNE REPONSE : un bloc NU a cote
        // d'un bloc decoupe est deja refuse au SCRIPT, par le controle de
        // recouvrement de C1.5. Une fenetre decoupee n'a donc aucune place ou
        // loger un bloc pleine fenetre, et la section venue du source n'a nulle
        // part ou aller — mieux vaut le lui dire que lui faire prononcer ce
        // refus-la, dont aucune de ses deux lignes n'est fautive.
        asmb::Object o = secObj("a.fo", {{"a", {1}}, {"c", {1}}});
        link::Image img = link::build({o},
            scr("MEMORY_MAP { CONFIG ext_w1<1> { w1 [OFFSET 0x0000, SIZE 0x2000] { SECTION a }\n"
                "                                w1 { SECTION c } } }"), pal3());
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("un bloc nu a cote d'un bloc decoupe est deja refuse au script",
           !img.ok && m.find("overlap") != std::string::npos);
    }
    {
        // UN PORT SANS SA VALEUR N'EST PAS UNE OFFRE, et cela vaut aussi du
        // chemin SCRIPT : le port est le meme par les deux fenetres la ou la
        // valeur differe, et la regle de retrait n'effacait que la valeur.
        profile::Profile pr = profile::parse(
            "WINDOW w0 [0x0000..0x3FFF]\n"
            "WINDOW w1 [0x4000..0x7FFF]\n"
            "BANK lo SIZE 0x4000 rw PAGE 0 STORE 0\n"
            "BANK hi SIZE 0x4000 rw PAGE 1 STORE 1\n"
            "CONFIG SET ram { both [CODE 0] { w0 lo  w1 hi } }\n"
            "SELECT ram = OUT 0x7F00, (PAGE << 3) | CODE\n", "m.prof");
        script::Script sc = scr("MEMORY_MAP { CONFIG both { w0 { SECTION x }\n"
                                "                          w1 { SECTION y } } }");
        std::set<std::string> gone;
        const std::map<std::string, int64_t> sy = link::switchSymbols(sc, pr, &gone);
        ok("la valeur ambigue est retiree, par le chemin script aussi",
           sy.find("__val_ram_both") == sy.end());
        ok("et son port ne lui survit pas", sy.find("__port_ram_both") == sy.end());
        ok("les deux noms sont rendus comme RETIRES, et non comme inexistants",
           gone.count("__val_ram_both") && gone.count("__port_ram_both"));

        // Et le diagnostic le dit : un symbole absent parce qu'il vaudrait deux
        // choses ne se distingue pas, pour qui l'emploie, d'un symbole qui n'a
        // jamais existe.
        asmb::Object o = secObj("a.fo", {{"x", {1}}, {"y", {2}}});
        asmb::Reloc r;
        r.frag = 0; r.offset = 0; r.kind = asmb::Reloc::Abs16; r.symbol = "__val_ram_both";
        o.relocs.push_back(r);
        link::Image img = link::build({o}, sc, pr);
        const std::string m = img.errors.empty() ? std::string() : img.errors[0].message;
        ok("l'EXTERN non resolu dit POURQUOI le symbole manque",
           !img.ok && m.find("two values depending on the window") != std::string::npos);
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
