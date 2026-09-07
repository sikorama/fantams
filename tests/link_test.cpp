// link_test.cpp - Tests du linker : des objets, une image
//
// Le gain de test de cette suite est qu'elle part d'objets FABRIQUES A LA MAIN.
// Verifier un recouvrement, une banque derivee ou un point d'entree ne demande
// plus d'ecrire un source Z80 qui les provoque, mais deux structures de dix
// lignes — et un test qui echoue nomme le linker, pas l'assembleur.
#include "link.h"

#include <cstdio>
#include <string>
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
        // Le recouvrement se voit AUSSI entre deux objets, et chacun nomme son
        // fichier : c'est tout l'interet de la table de sites concatenee.
        asmb::Object a = obj1(frag(0x8000, {1, 2}), "a.asm", 7);
        asmb::Object b = obj1(frag(0x8000, {3, 4}), "b.asm", 9);
        link::Image img = link::build({a, b});
        ok("un recouvrement inter-objets est vu", img.warnings.size() == 1);
        const std::string m = img.warnings.empty() ? std::string() : img.warnings[0].message;
        ok("il nomme le fichier ecrase", m.find("a.asm:7") != std::string::npos);
        ok("et il est rapporte sur l'ecrasant",
           !img.warnings.empty() && img.warnings[0].file == "b.asm");
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

    // --- Rien a lier --------------------------------------------------------
    {
        link::Image img = link::build({});
        ok("aucun objet donne une image vide et valide",
           img.ok && img.bin.empty() && img.blocks.empty() && img.banksWritten.empty());
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
