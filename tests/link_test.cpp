// link_test.cpp - Tests du linker : des objets, une image
//
// Le gain de test de cette suite est qu'elle part d'objets FABRIQUES A LA MAIN.
// Verifier un recouvrement, une banque derivee ou un point d'entree ne demande
// plus d'ecrire un source Z80 qui les provoque, mais deux structures de dix
// lignes — et un test qui echoue nomme le linker, pas l'assembleur.
#include "link.h"

#include <cstdio>
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

    // --- Rien a lier --------------------------------------------------------
    {
        link::Image img = link::build({});
        ok("aucun objet donne une image vide et valide",
           img.ok && img.bin.empty() && img.blocks.empty() && img.banksWritten.empty());
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
