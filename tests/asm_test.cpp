// asm_test.cpp - Tests de l'assembleur 2 passes
#include "asm.h"
#include "link.h"
#include "sym.h"

#include <map>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;

// --- Le harnais : assembler PUIS linker -------------------------------------
// L'assembleur ne rend plus d'adresses : il rend un OBJET, et c'est le linker
// qui pose les octets. Les assertions, elles, portent sur des octets a des
// adresses — ce qu'un auteur voit — et n'ont aucune raison de changer.
//
// Ce helper n'est donc PAS une API : l'exposer figerait dans l'interface de
// l'assembleur les six champs de placement que l'etage B existe pour en
// retirer. Il vit ici, dans le harnais, et nulle part ailleurs.
struct Built {
    asmb::Object obj;                        // l'objet, pour ce qui s'y teste directement
    link::Image img;                         // l'image, dont sort la table des symboles
    bool ok = true;
    std::vector<uint8_t> bin;
    uint16_t loadAddress = 0, runAddress = 0;
    std::map<std::string, int64_t> symbols;
    std::vector<asmb::Diagnostic> errors, warnings, prints;
    std::vector<uint8_t> image, coverage;    // les 128 K a plat, comme avant
    std::vector<int> banksWritten;
};

static Built build(const std::string &src, const char *file) {
    Built b;
    b.obj = asmb::assembleText(src, file);
    b.img = link::build({b.obj});
    const link::Image &img = b.img;
    const link::Flat flat = link::flatten(img);
    b.ok = b.obj.ok && img.ok;
    b.symbols = b.obj.symbols;
    b.errors = b.obj.errors;
    b.warnings = b.obj.warnings;
    b.prints = b.obj.prints;
    for (const auto &e : img.errors) b.errors.push_back(e);
    for (const auto &w : img.warnings) b.warnings.push_back(w);
    b.bin = img.bin;
    b.loadAddress = img.loadAddress;
    b.runAddress = img.runAddress;
    b.banksWritten = img.banksWritten;
    b.image = flat.bytes;
    b.coverage = flat.covered;
    return b;
}

static std::string hex(const std::vector<uint8_t> &v) {
    std::string s; char b[8];
    for (size_t i = 0; i < v.size(); ++i) { snprintf(b, sizeof b, "%02X", v[i]); if (i) s += ' '; s += b; }
    return s;
}

// Assemble `src`, vérifie octets + adresse de chargement.
static void chk(const char *desc, const std::string &src,
                std::initializer_list<uint8_t> expected, uint16_t load = 0) {
    Built o = build(src, "t.asm");
    std::vector<uint8_t> exp(expected);
    bool okLoad = (exp.empty() || o.loadAddress == load);
    if (!o.ok || o.bin != exp || !okLoad) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s\n    attendu @%04X [%s]\n    obtenu  @%04X [%s]\n",
               desc, load, hex(exp).c_str(), o.loadAddress, hex(o.bin).c_str());
        for (auto &e : o.errors) printf("    err %s:%d %s\n", e.file.c_str(), e.line, e.message.c_str());
    } else ++g_pass;
}

static void okc(const char *desc, bool cond) {
    if (cond) ++g_pass; else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

static void chkSym(const char *desc, const std::string &src, const char *sym, int64_t val) {
    Built o = build(src, "t.asm");
    auto it = o.symbols.find(sym);
    if (!o.ok || it == o.symbols.end() || it->second != val) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : %s attendu %lld obtenu %lld (ok=%d)\n",
               desc, sym, (long long)val, it == o.symbols.end() ? -1 : (long long)it->second, o.ok);
    } else ++g_pass;
}

// Compte les lignes d'un CSV, en-tete comprise.
static size_t csvLines(const std::string &t) {
    size_t n = 0;
    for (char c : t) if (c == '\n') ++n;
    return n;
}

// La ligne de la table qui commence par « nom, ». Chaine vide si absente.
static std::string symRow(const std::string &table, const std::string &name) {
    const std::string key = name + ",";
    size_t p = 0;
    while (p < table.size()) {
        size_t e = table.find('\n', p);
        if (e == std::string::npos) e = table.size();
        if (table.compare(p, key.size(), key) == 0) return table.substr(p, e - p);
        p = e + 1;
    }
    return std::string();
}

static void chkErr(const char *desc, const std::string &src) {
    Built o = build(src, "t.asm");
    if (o.ok) { ++g_fail; printf("  \033[31mFAIL\033[0m %s (aurait dû échouer)\n", desc); }
    else ++g_pass;
}

// Vérifie qu'un avertissement (bonne pratique) est bien émis, sans bloquer l'assemblage.
static void chkWarn(const char *desc, const std::string &src, bool expectWarning) {
    Built o = build(src, "t.asm");
    bool hasWarn = !o.warnings.empty();
    if (!o.ok || hasWarn != expectWarning) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : ok=%d warnings=%zu (attendu=%d)\n",
               desc, o.ok, o.warnings.size(), expectWarning);
    } else ++g_pass;
}

// --- Banques (ADR 0005 / ADR 0006) ------------------------------------------
static void chkBank(const char *desc, const std::string &src,
                    std::initializer_list<int> expectedExtra) {
    Built o = build(src, "t.asm");
    std::vector<int> exp(expectedExtra);
    if (!o.ok || o.banksWritten != exp) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : ok=%d banksWritten={", desc, o.ok);
        for (size_t k = 0; k < o.banksWritten.size(); ++k) printf("%s%d", k ? "," : "", o.banksWritten[k]);
        printf("}\n");
        for (auto &e : o.errors) printf("    err %s:%d %s\n", e.file.c_str(), e.line, e.message.c_str());
    } else ++g_pass;
}

int main() {
    printf("Tests assembleur 2 passes\n");

    // base + ORG
    chk("nop/ret", "  nop\n  ret\n", {0x00, 0xC9});
    chk("org", "  org 0x8000\n  ld a,1\n", {0x3E, 0x01}, 0x8000);

    // référence AVANT (le point clé des 2 passes)
    chk("forward jp",
        "  org 0x8000\nstart:\n  jp done\n  nop\ndone:\n  ret\n",
        {0xC3, 0x04, 0x80, 0x00, 0xC9}, 0x8000);
    chk("forward jr",
        "  org 0\n  jr next\nnext:\n  nop\n",
        {0x18, 0x00, 0x00}, 0);
    chk("backward ref",
        "  org 0x100\nloop:\n  djnz loop\n",
        {0x10, 0xFE}, 0x100); // -2

    // symboles / EQU / '='
    chkSym("label addr", "  org 0x4000\n  nop\nhere:\n  ret\n", "here", 0x4001);
    chk("equ usage", "VAL equ 0x42\n  ld a,VAL\n", {0x3E, 0x42});
    chk("equ colon", "VAL: equ 7\n  ld b,VAL\n", {0x06, 0x07});
    chk("assign =", "port = 0xFE\n  in a,(port)\n", {0xDB, 0xFE});
    chk("equ forward", "  ld hl,SIZE\nSIZE equ tail-head\nhead:\n  nop\ntail:\n",
        {0x21, 0x01, 0x00, 0x00}); // ld hl,1 (tail-head=1) + nop

    // $ = adresse courante
    chk("dollar", "  org 0x0100\n  dw $\n", {0x00, 0x01}, 0x0100);

    // directives data
    chk("db mixte", "  db 1,2,\"AB\",0\n", {0x01, 0x02, 0x41, 0x42, 0x00});
    chk("dw", "  dw 0x1234,0xABCD\n", {0x34, 0x12, 0xCD, 0xAB});
    chk("ds", "  ds 3\n", {0x00, 0x00, 0x00});
    chk("ds fill", "  ds 2,0xFF\n", {0xFF, 0xFF});
    chk("db expr", "  db 2*3+1, 1<<4\n", {0x07, 0x10});
    chk("db char", "  db 'A','Z'\n", {0x41, 0x5A});

    // label sans ':' (toléré, comme l'assembleur de référence) tant que le 1er mot n'est pas un mnémo/directive connu
    chk("label sans ':'", "start\n  ld a,1\n  jp start\n", {0x3E, 0x01, 0xC3, 0x00, 0x00});
    chkSym("label sans ':' addr", "  org 0x4000\nstart\n  nop\n", "start", 0x4000);
    // NOLIST/LIST : no-op (contrôle du listing seulement, comme l'assembleur de référence)
    chk("nolist no-op", "  nolist\n  ld a,1\n  list\n  ld b,2\n", {0x3E, 0x01, 0x06, 0x02});
    // BUILDSNA/BANKSET : no-op chez fantams (le split sur ':' est fait par pp.cpp en amont ;
    // ici chaque statement est déjà sur sa propre ligne, comme le reçoit vraiment asm.cpp).
    chk("BUILDSNA en-tête d'export (no-op)",
        "BUILDSNA V2\nBANKSET 0\nORG 0x8000\nRUN $\n  ld a,1\n", {0x3E, 0x01}, 0x8000);

    // avertissements de bonne pratique (non bloquants)
    chkWarn("warn label sans ':'", "start\n  nop\n", true);
    chkWarn("pas de warn label avec ':'", "start:\n  nop\n", false);
    chkWarn("warn instruction en colonne 1", "start:\nnop\n", true);
    chkWarn("pas de warn instruction indentée", "start:\n  nop\n", false);

    // labels locaux ".nom" : qualifiés par le dernier label global (comme l'assembleur de référence) ->
    // deux ".loop" sous deux globaux différents ne collisionnent pas.
    chkSym("label local .nom sous 2 globaux distincts (A)",
        "blockA:\n.loop:\n  nop\nblockB:\n.loop:\n  nop\n", "blockA.loop", 0);
    chkSym("label local .nom sous 2 globaux distincts (B)",
        "blockA:\n.loop:\n  nop\nblockB:\n.loop:\n  nop\n", "blockB.loop", 1);
    chkErr("label local .nom hors contexte -> non résolu",
        "blockA:\n.loop:\n  nop\n  ret\nblockB:\n  nop\n  ret\nblockC:\n  jp .loop\n");
    // référence qualifiée explicite "global.local" depuis un autre contexte
    chkSym("label local référencé via global.local", "blockA:\n.loop:\n  nop\n  jp blockA.loop\n", "blockA.loop", 0);

    // repli insensible à la casse (l'assembleur de référence ne distingue pas la casse) : résolu + avertissement
    chkWarn("repli casse : résolu avec avertissement", "Foo: nop\n  jp foo\n", true);
    chkErr("casse : rien à replier -> erreur si vraiment absent", "  jp doesNotExist\n");

    // align
    chk("align",
        "  org 0x4001\n  db 0xAA\n  align 4\n  db 0xBB\n",
        {0xAA, 0x00, 0x00, 0xBB}, 0x4001); // AA@4001, align->4004, BB@4004

    // programme complet réaliste
    chk("prog",
        "  org 0x8000\n"
        "  ld hl,msg\n"
        "loop:\n"
        "  ld a,(hl)\n"
        "  or a\n"
        "  ret z\n"
        "  inc hl\n"
        "  jr loop\n"
        "msg:\n"
        "  db \"Hi\",0\n",
        {0x21, 0x09, 0x80,   // ld hl,msg (msg=0x8009)
         0x7E,               // ld a,(hl)
         0xB7,               // or a
         0xC8,               // ret z
         0x23,               // inc hl
         0x18, 0xFA,         // jr loop (-6)
         0x48, 0x69, 0x00},  // "Hi",0
        0x8000);

    // erreurs
    chkErr("symbole indéfini", "  ld a,UNDEF\n");
    chkErr("label dupliqué", "foo:\n  nop\nfoo:\n  nop\n");
    chkErr("directive inconnue", "  bogus 1,2\n");

    // --- coverage et chevauchement (ADR 0012) ------------------------------
    {
        Built o = build("  org #8000\n  db 0,0\n", "t.asm");
        int n = 0;
        for (auto c : o.coverage) if (c) ++n;
        okc("coverage : deux zeros ecrits sont couverts", o.ok && n == 2 &&
            o.coverage[0x8000] && o.coverage[0x8001]);
        okc("coverage : le reste ne l'est pas", !o.coverage[0x7FFF] && !o.coverage[0x8002]);
        okc("coverage : pas d'avertissement sans chevauchement", o.warnings.empty());
    }
    {
        // deux ORG qui se recouvrent : un seul avertissement pour la plage, avec
        // les DEUX lignes en conflit nommees.
        Built o = build(
            "  org #8000\n  db 1,2,3,4\n  org #8001\n  db 9,9\n", "t.asm");
        bool one = o.warnings.size() == 1;
        std::string m = one ? o.warnings[0].message : std::string();
        okc("chevauchement : un seul avertissement pour la plage", one);
        okc("chevauchement : plage coalescee &8001-&8002",
            m.find("&8001-&8002") != std::string::npos);
        okc("chevauchement : nomme le site ecrase", m.find("t.asm:2") != std::string::npos);
        okc("chevauchement : rapporte sur le site ecrasant",
            one && o.warnings[0].line == 4);
    }
    {
        Built o = build("  org #8000\n  db 1\n  org #9000\n  db 1\n", "t.asm");
        okc("chevauchement : deux ORG disjoints n'en produisent pas", o.warnings.empty());
    }

    // --- Chaines : les deux delimiteurs, et la chaine decalee (ADR 0010) -----
    // Cas de reference du lot : chaque ligne exerce UNE construction, et les
    // refus en font partie autant que les acceptations — un message d'erreur
    // qui cesse de nommer son remplacant est une regression que rien d'autre
    // n'attrape.
    {
        // Les deux delimiteurs designent le meme objet : aucune ecriture ne
        // marche d'un cote et echoue de l'autre.
        chk("chaine : db simple quote", "  org #8000\n  db 'hi'\n", {0x68, 0x69}, 0x8000);
        chk("chaine : db double quote", "  org #8000\n  db \"hi\"\n", {0x68, 0x69}, 0x8000);
        chk("chaine : dm accepte aussi le simple quote", "  org #8000\n  dm 'hi'\n", {0x68, 0x69}, 0x8000);

        // Le delimiteur OPPOSE est du contenu ordinaire, sans echappement.
        chk("chaine : guillemet dans un litteral simple", "  org #8000\n  db 'a\"b'\n",
            {0x61, 0x22, 0x62}, 0x8000);
        chk("chaine : apostrophe dans un litteral double", "  org #8000\n  db \"a'b\"\n",
            {0x61, 0x27, 0x62}, 0x8000);

        // Un litteral d'UN octet vaut son code, quel que soit le delimiteur.
        chk("chaine : 'x' en expression", "  org #8000\n  ld a,'x'\n", {0x3E, 0x78}, 0x8000);
        chk("chaine : \"x\" en expression", "  org #8000\n  ld a,\"x\"\n", {0x3E, 0x78}, 0x8000);
        chk("chaine : arithmetique sur un litteral d'un octet",
            "  org #8000\n  db 'a'+1\n", {0x62}, 0x8000);

        // Litteral vide : zero octet emis, et l'element suivant reste en place.
        chk("chaine : litteral vide n'emet rien", "  org #8000\n  db '',#AA\n", {0xAA}, 0x8000);

        // Chaine decalee : la queue s'applique a CHAQUE octet.
        chk("chaine decalee : db 'hello'-'a'", "  org #8000\n  db 'hello'-'a'\n",
            {0x07, 0x04, 0x0B, 0x0B, 0x0E}, 0x8000);
        chk("chaine decalee : queue composee", "  org #8000\n  db 'hello'-'a'+1\n",
            {0x08, 0x05, 0x0C, 0x0C, 0x0F}, 0x8000);
        chk("chaine decalee : les deux delimiteurs, meme resultat",
            "  org #8000\n  db \"hello\"-\"a\"\n", {0x07, 0x04, 0x0B, 0x0B, 0x0E}, 0x8000);
        chk("chaine decalee : tout operateur binaire, pas seulement + et -",
            "  org #8000\n  db 'ab'*2\n", {0xC2, 0xC4}, 0x8000);
        chk("chaine decalee : le decalage se masque sur 8 bits",
            "  org #8000\n  db 'a'-'z'\n", {0xE7}, 0x8000);

        // --- Refus ----------------------------------------------------------
        // Contexte SCALAIRE : aucune valeur n'existe, et l'assembleur de référence y repond par un
        // zero silencieux qu'on se refuse a reproduire.
        chkErr("refus : ld hl,'ab' (aucune convention d'endianness)",
               "  org #8000\n  ld hl,'ab'\n");
        chkErr("refus : ld hl,\"\" (litteral vide sans valeur)",
               "  org #8000\n  ld hl,\"\"\n");
        chkErr("refus : dw n'est pas un contexte de chaine decalee",
               "  org #8000\n  dw 'hello'-'a'\n");
        // Les parentheses annoncent une expression : le litteral y redevient un
        // operande, donc sans valeur.
        chkErr("refus : db ('hello')-'a' n'est pas une chaine decalee",
               "  org #8000\n  db ('hello')-'a'\n");
        // Le litteral doit etre en TETE de l'element.
        chkErr("refus : db 1+'hello' (litteral pas en tete)",
               "  org #8000\n  db 1+'hello'\n");
        chkErr("refus : deux litteraux dans un element",
               "  org #8000\n  db 'ab'-'cd'\n");
        chkErr("refus : litteral non termine", "  org #8000\n  db 'hello\n");
        chkErr("refus : PRINT n'accepte pas une chaine decalee",
               "  org #8000\n  print 'hello'-'a'\n");
        chkErr("refus : CHARSET", "  org #8000\n  charset '0123',0\n");
        // Une expression qui echoue a deja produit son erreur : PRINT ne doit pas
        // afficher en plus une valeur fabriquee, que le lecteur prendrait pour un
        // resultat.
        chkErr("refus : PRINT sur expression invalide", "  org #8000\n  print 1+\n");
        chkErr("refus : PRINT sur symbole inconnu", "  org #8000\n  print nexistepas\n");
    }
    {
        // Le choix du delimiteur n'est pas un avertissement : il ne nomme
        // aucune ambiguite (ADR 0010), contrairement au label sans ':'.
        chkWarn("chaine : le simple quote n'avertit pas",
                "  org #8000\n  db 'hello'\n", false);
        chkWarn("chaine : le double quote n'avertit pas non plus",
                "  org #8000\n  db \"hello\"\n", false);
    }
    {
        // Le refus de CHARSET nomme son remplacant, comme BANK ou TICKER.
        Built o = build("  org #8000\n  charset '0123',0\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message.find("asset encoding") != std::string::npos;
        okc("refus : CHARSET nomme son remplacant", named);
    }

    // ADR 0015 : aucun identifiant utilisateur ne porte un nom du langage ni de la

    // machine. Les deux positions que l'assembleur possède : symbole et label.

    chkErr("registre en nom de constante", "hl equ 5\n");

    chkErr("registre en nom de variable", "c = 7\n");

    chkErr("condition en nom de constante", "nz equ 1\n");

    chkErr("mnémonique en label", "call: nop\n");

    chkErr("directive en label", "org: nop\n");

    chkErr("registre en label", "hl: nop\n");

    chkSym("label libre", "boucle: nop\n", "boucle", 0);

    chkSym("label local non concerné", "g: nop\n.b: nop\n", "g.b", 1);


    {
        // --- « org b<n>:adresse » (ADR 0005) --------------------------------
        // Le prefixe designe le RANGEMENT, le nombre qui suit reste l'adresse
        // LOGIQUE : c'est elle que prend le label.
        chkSym("b2 : le label prend l'adresse logique",
               "  org b2:#8000\nlab: db 1\n", "lab", 0x8000);
        chkSym("b4 : idem hors des 64K de base",
               "  org b4:#4000\nlab: db 1\n", "lab", 0x4000);
        chk("banque de base : le binaire est inchange",
            "  org b2:#8000\n  db 1,2,3\n", {1, 2, 3}, 0x8000);
        // L'offset vaut « adresse & 0x3FFF » : b2:#8000 range au meme endroit que
        // le #8000 nu, puisque 0x8000 >> 14 vaut 2.
        chk("b2:#8000 equivaut au #8000 nu", "  org #8000\n  db 9\n", {9}, 0x8000);
        // `banksWritten` liste TOUTES les banques ecrites : c'est l'appelant qui en
        // tire la taille du dump (64 ou 128 Ko) et le refus au-dela de la 7.
        chkBank("banque de base", "  org b2:#8000\n  db 1\n", {2});
        chkBank("sans prefixe, la banque suit l'adresse", "  org #8000\n  db 1\n", {2});
        chkBank("banque haute", "  org b4:#4000\n  db 1\n", {4});
        chkBank("plusieurs banques, triees",
                "  org b5:#4000\n  db 1\n  org b4:#4000\n  db 2\n", {4, 5});
        chkBank("base et extension melangees",
                "  org #0000\n  db 1\n  org b6:#4000\n  db 2\n", {0, 6});
        // Au-dela de la banque 7, l'assemblage marche : c'est l'EXPORT qui refuse.
        chkBank("banque 8 : assemblee quand meme", "  org b8:#4000\n  db 1\n", {8});

        // L'image porte les banques 0..7 a plat : (banque, offset) -> b*0x4000+o.
        {
            Built o = build("  org b5:#4000\n  db #AB\n", "t.asm");
            okc("image : 128K", o.image.size() == 131072);
            okc("image : l'octet est en banque 5, offset 0", o.image[5 * 0x4000] == 0xAB);
            okc("image : la coverage suit", o.coverage[5 * 0x4000] != 0);
            okc("image : rien ailleurs", o.coverage[0x4000] == 0);
        }

        // La banque est REMANENTE, et un ORG nu qui en herite une haute avertit :
        // un prefixe oublie deplacerait le bloc sans aucun diagnostic.
        chkWarn("ORG nu heritant une banque haute : avertit",
                "  org b4:#4000\n  db 1\n  org #c000\n  db 2\n", true);
        chkWarn("ORG nu sans banque explicite : rien",
                "  org #4000\n  db 1\n  org #c000\n  db 2\n", false);
        chkWarn("b0 ramene dans les 64K de base, sans avertir",
                "  org b4:#4000\n  db 1\n  org b0:#0000\n  db 2\n", false);

        // Le cout du masquage, assume par l'ADR 0005 : b4:#4000 et b4:#8000 se
        // rangent au meme offset. C'est le detecteur de recouvrement qui le dit.
        chkWarn("masquage : deux adresses logiques, un seul rangement",
                "  org b4:#4000\n  db 1\n  org b4:#8000\n  db 2\n", true);

        chkErr("prefixe qui n'est pas une banque", "  org x2:#4000\n  db 1\n");
        chkErr("prefixe sans adresse", "  org b4:\n  db 1\n");
    }


    // --- ORG a deux parametres : logique et rangement (ADR 0005) --------------
    //
    // Semantique usuelle, mesuree contre l'assembleur de référence : le PREMIER parametre est l'adresse
    // logique — celle des labels, celle pour laquelle le code est assemble — et le
    // SECOND l'adresse de rangement, ou les octets sont reellement ecrits en
    // attendant qu'un chargeur les recopie.
    printf("\n-- ORG deplace (logique, rangement) --\n");
    {
        const char *src = "  org #2000,#3000\nstart:\n  ld hl,start\n";
        Built o = build(src, "t.asm");
        okc("deplace : le label vaut l'adresse LOGIQUE", o.symbols["start"] == 0x2000);
        okc("deplace : l'octet est range a l'adresse de RANGEMENT",
            o.image[0x3000] == 0x21 && o.image[0x3001] == 0x00 && o.image[0x3002] == 0x20);
        okc("deplace : rien a l'adresse logique", o.coverage[0x2000] == 0);
        okc("deplace : loadAddress est le rangement", o.loadAddress == 0x3000);
    }
    // ALIGN aligne le LOGIQUE (comme l'assembleur de référence) : c'est l'adresse ou le code tournera
    // apres recopie. Le rangement suit du meme ecart, donc n'est pas aligne.
    chkSym("deplace : ALIGN aligne le logique",
           "  org #2000,#3000\n  nop\n  align 16\naligned:\n  nop\n", "aligned", 0x2010);
    // Le deplacement N'EST PAS REMANENT : un ORG nu le remet a zero (comme l'assembleur de référence).
    {
        Built o = build("  org #2000,#3000\n  nop\n  org #5000\n  db #42\n", "t.asm");
        okc("deplace : un ORG nu remet le deplacement a zero", o.image[0x5000] == 0x42);
        okc("deplace : et n'ecrit pas a l'ancien ecart", o.coverage[0x6000] == 0);
    }
    // Le chevauchement se produit AU RANGEMENT : c'est la que les octets s'ecrasent.
    chkWarn("deplace : le chevauchement est detecte au rangement",
            "  org #2000,#3000\n  db 1\n  org #3000\n  db 2\n", true);
    // Un RUN qui tombe dans un bloc deplace demarre sur de la memoire vide : le PC
    // reste celui que la source demande, mais le silence serait une panne sans
    // diagnostic.
    chkWarn("deplace : RUN dans un bloc deplace avertit",
            "  org #2000,#3000\nstart:\n  nop\n  run start\n", true);
    chkWarn("deplace : RUN hors bloc deplace n'avertit pas",
            "  org #2000,#3000\n  nop\n  org #4000\nboot:\n  nop\n  run boot\n", false);
    // Le prefixe de banque qualifie le RANGEMENT : il se porte donc sur le DERNIER
    // parametre. Sur le premier d'une forme a deux, il est refuse — le rangement
    // serait decrit de part et d'autre de l'adresse logique.
    {
        Built o = build("  org #4000,b4:#100\n  db #AB\n", "t.asm");
        okc("deplace : prefixe sur le rangement", o.ok && o.image[4 * 0x4000 + 0x100] == 0xAB);
    }
    chkErr("deplace : prefixe sur le premier parametre", "  org b4:#4000,#100\n  db 1\n");
    chkErr("deplace : deux prefixes", "  org b4:#4000,b5:#100\n  db 1\n");
    chkErr("deplace : trois parametres", "  org #4000,#100,#200\n  db 1\n");

    // --- Table des symboles (ADR 0019) ---------------------------------------
    printf("\n-- Table des symboles (--sym) --\n");
    {
        const std::string src =
            "SCREEN equ #C000\n"
            "BIG    equ 1<<20\n"
            "MINUS  equ -1\n"
            "count = 1\n"
            "count = 5\n"
            "  org #8000\n"
            "main:\n"
            "  nop\n"
            "  org #4000,b4:#100\n"
            "far:\n"
            "  nop\n";
        Built o = build(src, "t.asm");
        const std::string t = sym::format(o.img);

        // L'en-tete est une VRAIE ligne CSV : les noms de colonnes SONT la version.
        okc("sym : en-tete exacte", t.rfind("name,type,section,value,bank,store,file,line\n", 0) == 0);
        // 5 symboles : 2 labels + 3 constantes. La variable 'count' n'y est PAS.
        okc("sym : une ligne par symbole, plus l'en-tete", csvLines(t) == 6);
        okc("sym : la variable n'est pas exportee", symRow(t, "count").empty());

        okc("sym : un label porte son type, sa valeur et son rangement",
            symRow(t, "main") == "main,label,-,0x8000,2,0x8000,t.asm,7");
        // Bloc deplace ET en banque : la valeur reste logique, le rangement suit.
        okc("sym : un label deplace separe valeur et rangement",
            symRow(t, "far") == "far,label,-,0x4000,4,0x100,t.asm,10");
        // Une constante n'habite nulle part : ni banque ni rangement.
        okc("sym : une constante n'a ni banque ni rangement",
            symRow(t, "SCREEN") == "SCREEN,const,-,0xC000,-,-,t.asm,1");
        // Non masquee, et signee : c'est la valeur que l'assembleur a utilisee.
        okc("sym : une constante n'est pas masquee en 16 bits",
            symRow(t, "BIG") == "BIG,const,-,0x100000,-,-,t.asm,2");
        okc("sym : une constante negative garde son signe",
            symRow(t, "MINUS") == "MINUS,const,-,-0x1,-,-,t.asm,3");

        // Tri : banque, puis rangement, puis nom ; les constantes en QUEUE. Un
        // consommateur qui lit jusqu'a la premiere banque « - » a tous les
        // symboles adressables.
        const size_t pMain = t.find("\nmain,"), pFar = t.find("\nfar,"),
                     pBig = t.find("\nBIG,"), pMinus = t.find("\nMINUS,");
        okc("sym : tri par banque puis rangement", pMain < pFar);
        okc("sym : les constantes forment la queue", pFar < pBig);
        okc("sym : les constantes sont triees par nom", pBig < pMinus);
    }
    {
        // Les noms sont ceux que l'assembleur connait, sans retouche. Ici le cas
        // QUALIFIE : « .inner » sort en « top.inner ». Le cas MANGLE (« @retry__2 »)
        // se produit au preprocesseur, que `assembleText` ne fait pas tourner — il
        // se verifie de bout en bout au CLI, pas ici.
        Built o = build(
            "  org #8000\n@loop:\n  nop\ntop:\n.inner:\n  nop\n", "t.asm");
        const std::string t = sym::format(o.img);
        okc("sym : un label local sort qualifie", !symRow(t, "top.inner").empty());
    }
    {
        // Le champ fichier est guillemete SEULEMENT s'il en a besoin : sans ca, un
        // chemin a virgule produirait une ligne a huit champs dans un fichier a sept.
        Built o = build("  org #8000\nmain:\n  nop\n", "mon,brouillon.asm");
        const std::string t = sym::format(o.img);
        okc("sym : un chemin a virgule est guillemete",
            t.find(",\"mon,brouillon.asm\",2\n") != std::string::npos);
    }

    // --- SECTION : la section dans la table des symboles (§4.1, §4.6) --------
    {
        // Un label habite la section courante, et la table le dit : c'est ce qui
        // permet a un consommateur de savoir de quelle unite relogeable il parle.
        Built o = build(
            "  section tables_data, \"ro\"\n"
            "  org #8000\n"
            "my_table:\n"
            "  nop\n", "t.asm");
        const std::string t = sym::format(o.img);
        okc("sym : un label porte sa section",
            symRow(t, "my_table") == "my_table,label,tables_data,0x8000,2,0x8000,t.asm,3");
    }
    {
        // Une constante n'habite nulle part — c'est deja ce que disent `bank` et
        // `store`. Declaree dans une section, elle n'en herite donc PAS : elle
        // n'est pas une adresse, et rien ne la reloge.
        Built o = build(
            "  section tables_data, \"ro\"\n"
            "  org #8000\n"
            "WIDTH equ 80\n"
            "  nop\n", "t.asm");
        const std::string t = sym::format(o.img);
        okc("sym : une constante n'herite pas de la section",
            symRow(t, "WIDTH") == "WIDTH,const,-,0x50,-,-,t.asm,3");
    }

    // Les trois types du §4.1 sont la SEULE semantique materielle que
    // l'assembleur connaisse. Un quatrieme se refuse en nommant les trois.
    chkErr("section : un type inconnu est refuse",
           "  section audio, \"rox\"\n  org #8000\n  nop\n");
    // Le type est OBLIGATOIRE : sans lui, l'assembleur ne saurait ni refuser une
    // ecriture, ni dire au linker si la section porte des octets.
    chkErr("section : un type manquant est refuse",
           "  section audio\n  org #8000\n  nop\n");

    // --- Ecriture en "ro", detectee statiquement (§4.2) ----------------------
    // Connaissant le type de la section qui porte chaque symbole, l'assembleur
    // refuse une ecriture vers une section en lecture seule. C'est l'exemple du
    // §4.2, mot pour mot.
    chkErr("ro : « ld (nn),a » vers une section \"ro\" est refuse",
           "  section tables_data, \"ro\"\n"
           "  org #8000\n"
           "mon_tableau:\n"
           "  db 1, 2, 3, 4\n"
           "  section execution, \"ro\"\n"
           "  ld a, 5\n"
           "  ld (mon_tableau), a\n");

    // Une section se REOUVRE — c'est ainsi qu'on alterne code et donnees — mais
    // pas avec un autre type : sans ce refus, « ro » puis « rw » sous le meme nom
    // desarmerait le controle ci-dessus en silence.
    chkErr("ro : rouvrir une section avec un autre type est refuse",
           "  section data, \"ro\"\n"
           "  org #8000\n"
           "mon_tableau:\n"
           "  db 1\n"
           "  section data, \"rw\"\n"
           "  ld (mon_tableau), a\n");

    // Le refus NE MORD PAS au-dela : une section "rw" s'ecrit, et une section
    // "ro" se LIT — c'est meme sa raison d'etre.
    chk("ro : ecrire dans une section \"rw\" est permis",
        "  section vars, \"rw\"\n  org #8000\ncompteur:\n  db 0\n"
        "  ld (compteur), a\n", {0x00, 0x32, 0x00, 0x80}, 0x8000);
    chk("ro : lire une section \"ro\" est permis",
        "  section tables, \"ro\"\n  org #8000\ntable:\n  db 0\n"
        "  ld a, (table)\n", {0x00, 0x3A, 0x00, 0x80}, 0x8000);
    // Une adresse litterale ne designe aucun symbole : rien a controler.
    chk("ro : une adresse en dur n'est pas controlee",
        "  section tables, \"ro\"\n  org #8000\n  ld (#C000), a\n", {0x32, 0x00, 0xC0}, 0x8000);
    {
        // Le diagnostic cite l'instruction et NOMME la section fautive : celui qui
        // le lit doit savoir laquelle des deux corriger.
        Built o = build(
            "  section tables_data, \"ro\"\n  org #8000\nmon_tableau:\n  db 1\n"
            "  ld (mon_tableau + 1), hl\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message == "\"ld (nn), hl\" writes into read-only section 'tables_data'";
        okc("ro : le diagnostic cite l'instruction et nomme la section", named);
    }

    // --- Le plafond de taille declare (§4.1) ---------------------------------
    //
    // Le depassement sort A L'ASSEMBLAGE, sans attendre le linkage. Le message
    // nomme la section et donne les deux tailles : celui qui le lit doit savoir
    // de combien il deborde, pas seulement qu'il deborde.
    {
        Built o = build(
            "  section audio, \"ro\", 4\n  org #8000\n  db 1, 2, 3, 4, 5\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message ==
                         "Section 'audio' exceeds maximum declared size (0x5 > 0x4 bytes)";
        okc("section : le depassement du plafond est refuse, avec ses deux tailles", named);
    }
    // Le refus ne mord pas A `max` exactement : un plafond est une taille
    // permise, pas la premiere taille refusee.
    chk("section : la taille exactement egale au plafond est acceptee",
        "  section audio, \"ro\", 4\n  org #8000\n  db 1, 2, 3, 4\n",
        {0x01, 0x02, 0x03, 0x04}, 0x8000);
    // La taille est CUMULEE sur les reouvertures, et non l'etendue des adresses :
    // deux octets ici, deux octets la, et le plafond de trois est franchi — alors
    // meme que chaque ouverture, prise seule, tient.
    chkErr("section : la taille se cumule sur les reouvertures",
           "  section audio, \"ro\", 3\n  org #8000\n  db 1, 2\n"
           "  section code, \"ro\"\n  nop\n"
           "  section audio, \"ro\"\n  db 3, 4\n");
    // Ce qui n'emet pas ne compte pas : `align` et `boundary` avancent `pc_` sans
    // ecrire, et a cet etage le remplissage n'existe pas.
    chk("section : un align ne compte pas dans la taille",
        "  section audio, \"ro\", 2\n  org #8000\n  db 1\n  align 16\n  db 2\n",
        {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}, 0x8000);
    // Le plafond decide d'un refus : il doit etre connu quand les octets se
    // comptent, donc des la passe 1.
    chkErr("section : un plafond non evaluable en passe 1 est refuse",
           "  section audio, \"ro\", plus_tard\n  org #8000\n  db 1\n"
           "plus_tard equ 4\n");
    // Meme regle que pour le type : le laisser relever a la reouverture
    // desarmerait le controle en silence.
    chkErr("section : rouvrir une section avec un autre plafond est refuse",
           "  section audio, \"ro\", 4\n  org #8000\n  db 1\n"
           "  section audio, \"ro\", 8\n  db 2\n");
    chkErr("section : rouvrir avec un plafond ce qui n'en avait pas est refuse",
           "  section audio, \"ro\"\n  org #8000\n  db 1\n"
           "  section audio, \"ro\", 8\n  db 2\n");
    // Rouvrir SANS plafond conserve celui de la premiere declaration : c'est la
    // forme normale de l'alternance code / donnees.
    chkErr("section : le plafond survit a une reouverture qui ne le repete pas",
           "  section audio, \"ro\", 2\n  org #8000\n  db 1\n"
           "  section audio, \"ro\"\n  db 2, 3\n");
    // Un quatrieme argument accepte et ignore serait le pire des etats : son
    // auteur croirait avoir dit quelque chose.
    chkErr("section : un quatrieme argument est refuse",
           "  section audio, \"ro\", 4, 8\n  org #8000\n  db 1\n");

    // --- Une section "uninit" n'emet pas d'octet (§4.1) ----------------------
    //
    // `ds` y est exactement son usage : RESERVER. L'adresse avance, la coverage
    // ne bouge pas, et rien n'entre dans le binaire — le linker n'aurait nulle
    // part ou mettre des octets qu'une zone reservee porterait.
    chk("uninit : ds reserve sans rien emettre",
        "  section vars, \"uninit\"\n  org #8000\nbuffer:\n  ds 16\n", {});
    chkSym("uninit : la reservation fait avancer l'adresse",
           "  section vars, \"uninit\"\n  org #8000\nbuffer:\n  ds 16\napres:\n",
           "apres", 0x8010);
    // Ce qui est reserve n'etend pas l'image plate : le binaire commence au
    // premier octet REELLEMENT ecrit, et pas avant.
    chk("uninit : la reservation n'entre pas dans le binaire",
        "  section vars, \"uninit\"\n  org #8000\n  ds 4\n"
        "  section code, \"ro\"\n  org #9000\n  nop\n", {0x00}, 0x9000);
    // Une place reservee COMPTE : c'est la seule information qu'une section
    // "uninit" donne au linker, et sans elle son plafond ne servirait a rien.
    {
        Built o = build(
            "  section vars, \"uninit\", 8\n  org #8000\n  ds 16\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message ==
                         "Section 'vars' exceeds maximum declared size (0x10 > 0x8 bytes)";
        okc("uninit : la place reservee compte dans le plafond", named);
    }
    // Le refus nomme le TYPE de la section, qui est la raison ; la ligne citee
    // dit deja laquelle des directives corriger.
    {
        Built o = build(
            "  section vars, \"uninit\"\n  org #8000\n  db 1\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message ==
                         "section 'vars' is \"uninit\": it reserves space and emits no bytes "
                         "(use `ds` to reserve, or declare the section \"rw\")";
        okc("uninit : db y est refuse, en nommant le type de la section", named);
    }
    chkErr("uninit : dw y est refuse",
           "  section vars, \"uninit\"\n  org #8000\n  dw #1234\n");
    chkErr("uninit : une instruction y est refusee",
           "  section vars, \"uninit\"\n  org #8000\n  ld a, 5\n");
    // Une ligne qui emet cent octets n'a qu'une faute a corriger.
    {
        Built o = build(
            "  section vars, \"uninit\"\n  org #8000\n  db \"bonjour\"\n", "t.asm");
        okc("uninit : le refus est dit une fois par ligne, pas une fois par octet",
            o.errors.size() == 1);
    }
    // « ds 16,#FF » laisserait croire a une zone initialisee : la refuser plutot
    // que l'ignorer.
    chkErr("uninit : ds n'y prend pas de valeur de remplissage",
           "  section vars, \"uninit\"\n  org #8000\n  ds 16, #FF\n");
    // Le refus NE MORD PAS au-dela : ailleurs, `ds` emet toujours son
    // remplissage, et une section "rw" s'ecrit.
    chk("uninit : ailleurs, ds emet toujours son remplissage",
        "  section vars, \"rw\"\n  org #8000\n  ds 3, #FF\n", {0xFF, 0xFF, 0xFF}, 0x8000);

    // --- ASSERT_SIZE : un plafond sur une sous-zone (§4.1) -------------------
    //
    // La zone est EXPLICITE, sur le modele de BOUNDARY : « depuis le dernier
    // label » se lirait aussi bien, mais un label insere au milieu changerait ce
    // qui est mesure sans que personne l'ait demande.
    {
        Built o = build(
            "  org #8000\n  assert_size 2\ntable:\n  db 1, 2, 3\n  end_assert_size\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message ==
                         "Block 'table' exceeds its asserted size (0x3 > 0x2 bytes)";
        okc("assert_size : le depassement est refuse, et le bloc est nomme", named);
    }
    // Sans label, la zone est designee par son adresse : il faut bien pouvoir la
    // retrouver.
    {
        Built o = build(
            "  org #8000\n  assert_size 2\n  db 1, 2, 3\n  end_assert_size\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message ==
                         "Block at &8000 exceeds its asserted size (0x3 > 0x2 bytes)";
        okc("assert_size : une zone sans label est designee par son adresse", named);
    }
    // La taille exactement egale passe, et la zone s'assemble comme si de rien
    // n'etait : ASSERT_SIZE mesure, il ne deplace pas.
    chk("assert_size : la taille exactement egale est acceptee, et n'ecarte rien",
        "  org #8000\n  assert_size 3\ntable:\n  db 1, 2, 3\n  end_assert_size\n  db 4\n",
        {0x01, 0x02, 0x03, 0x04}, 0x8000);
    // A la difference de BOUNDARY, rien ne s'oppose a l'imbrication : la zone est
    // mesuree par difference d'adresses, il n'y a pas de mesure prealable qu'un
    // bloc interne arreterait.
    chk("assert_size : les zones s'imbriquent",
        "  org #8000\n  assert_size 4\n  db 1\n"
        "  assert_size 2\n  db 2, 3\n  end_assert_size\n  db 4\n  end_assert_size\n",
        {0x01, 0x02, 0x03, 0x04}, 0x8000);
    chkErr("assert_size : la zone interne est mesuree pour elle-meme",
           "  org #8000\n  assert_size 8\n  db 1\n"
           "  assert_size 1\n  db 2, 3\n  end_assert_size\n  end_assert_size\n");
    // Une mesure de BOUNDARY repasse sur les lignes du bloc : le controle ne doit
    // pas y etre fait deux fois.
    {
        Built o = build(
            "  org #8000\n  boundary 256\n  assert_size 1\n  db 1, 2\n"
            "  end_assert_size\n  end_boundary\n", "t.asm");
        okc("assert_size : dans un BOUNDARY, le controle n'est fait qu'une fois",
            o.errors.size() == 1);
    }
    chkErr("assert_size : une zone jamais fermee est refusee",
           "  org #8000\n  assert_size 4\n  db 1\n");
    chkErr("assert_size : une fermeture sans ouverture est refusee",
           "  org #8000\n  db 1\n  end_assert_size\n");
    // Le plafond decide d'un refus des la passe 1, comme celui d'une section.
    chkErr("assert_size : une taille non evaluable en passe 1 est refusee",
           "  org #8000\n  assert_size plus_tard\n  db 1\n  end_assert_size\n"
           "plus_tard equ 4\n");

    // --- ADR 0020 : ce que l'assembleur TOLERE, et ce qu'il refuse -----------
    //
    // L'invariant de l'ADR 0017 : aucune source ne doit etre assemblable seulement
    // apres passage par un outil de mise en forme. Ces formes arrivent donc ici
    // SANS preprocesseur — c'est ce chemin direct qu'exerce asm_test — et
    // l'assembleur doit les prendre telles quelles.
    chk("ex hl,de tolere",   "  ex hl,de\n",   {0xEB});
    chk("ex hl,(sp) tolere", "  ex hl,(sp)\n", {0xE3});
    chk("ex ix,(sp) tolere", "  ex ix,(sp)\n", {0xDD, 0xE3});
    chk("ex af,af tolere",   "  ex af,af\n",   {0x08});
    chk("jp hl tolere",      "  jp hl\n",      {0xE9});
    chk("jp ix tolere",      "  jp ix\n",      {0xDD, 0xE9});
    chk("le canon marche toujours", "  ex de,hl\n  jp (hl)\n  ex af,af'\n", {0xEB, 0xE9, 0x08});
    // L'avertissement de « ex af,af » appartient au preprocesseur, seul etage a voir
    // la source telle qu'elle est ecrite (ADR 0017). Ici, silence — comme `defb`.
    chkWarn("ex af,af ne dit rien a l'assembleur", "  ex af,af\n", false);

    // Les refus. Chacun nomme sa raison plutot que « unrecognized form » : ces
    // formes existent ailleurs, et celui qui les ecrit les croit valides.
    chkErr("inc hl,de refuse", "  inc hl,de\n");   // rendait UN octet, en silence
    chkErr("dec bc,de refuse", "  dec bc,de\n");
    chkErr("ld hl,sp refuse",  "  ld hl,sp\n");    // arithmetique inventee, carry ecrase
    chkErr("rlc hl refuse",    "  rlc hl\n");      // une routine, pas une orthographe
    chkErr("srl de refuse",    "  srl de\n");
    chkErr("rst z,#38 refuse", "  rst z,#38\n");   // deux instructions qui PARTAGENT un octet
    // Les formes 8 bits voisines restent intactes : le refus ne mord pas au-dela.
    chk("inc hl seul",   "  inc hl\n",   {0x23});
    chk("rlc h",         "  rlc h\n",    {0xCB, 0x04});
    chk("rlc (ix+1)",    "  rlc (ix+1)\n", {0xDD, 0xCB, 0x01, 0x06});
    chk("rst #38",       "  rst #38\n",  {0xFF});
    chk("ld hl,#1234",   "  ld hl,#1234\n", {0x21, 0x34, 0x12});

    // --- BOUNDARY : un bloc auto-mesure qui ne croise pas une frontiere (§5) --
    // La regle est unique : emettre a la suite si le bloc tient entierement dans
    // la page courante, sinon sauter au debut de la suivante.
    chkSym("BOUNDARY : le bloc tient dans la page, rien ne bouge",
           "  org #2F00\n"
           "  BOUNDARY 256\n"
           "my_table:\n"
           "  dw #1111\n"
           "  dw #2222\n"
           "  db #FF\n"
           "  END_BOUNDARY\n", "my_table", 0x2F00);

    // A &2FFE, les 5 octets ne tiennent pas dans les deux octets restants : le
    // bloc part en &3000, sans que l'auteur ait eu a mesurer sa table.
    chkSym("BOUNDARY : le bloc ne tient pas, il saute a la page suivante",
           "  org #2FFE\n"
           "  BOUNDARY 256\n"
           "my_table:\n"
           "  dw #1111\n"
           "  dw #2222\n"
           "  db #FF\n"
           "  END_BOUNDARY\n", "my_table", 0x3000);

    // Un bloc plus grand que sa frontiere ne peut JAMAIS satisfaire la regle :
    // c'est une erreur d'assemblage, pas un remplissage sans fin.
    chkErr("BOUNDARY : un bloc plus grand que sa frontiere est refuse",
           "  org #2F00\n"
           "  BOUNDARY 4\n"
           "  db 1, 2, 3, 4, 5\n"
           "  END_BOUNDARY\n");

    {
        // Le diagnostic NOMME le bloc et ses deux tailles, et pointe la ligne du
        // BOUNDARY : c'est la seule que son auteur peut corriger.
        Built o = build(
            "  org #2F00\n"
            "  BOUNDARY 256\n"
            "my_table:\n"
            "  ds 300\n"
            "  END_BOUNDARY\n", "t.asm");
        bool named = !o.errors.empty() &&
                     o.errors[0].message.find("'my_table'") != std::string::npos &&
                     o.errors[0].message.find("300 bytes") != std::string::npos &&
                     o.errors[0].message.find("256 bytes") != std::string::npos &&
                     o.errors[0].line == 2;
        okc("BOUNDARY : le depassement nomme le bloc et sa ligne", named);
    }

    // Le saut n'EMET rien : il avance l'adresse, comme ALIGN. Les octets sautes
    // restent hors coverage, ce dont depend la fusion avec une base (ADR 0012).
    chk("BOUNDARY : le saut n'emet aucun octet de remplissage",
        "  org #2FFE\n"
        "  db #AA\n"
        "  BOUNDARY 256\n"
        "  db #BB, #CC, #DD\n"
        "  END_BOUNDARY\n", {0xAA, 0x00, 0xBB, 0xCC, 0xDD}, 0x2FFE);

    // Les deux fermetures qui manquent. Un END_BOUNDARY orphelin et un BOUNDARY
    // jamais ferme sont des fautes de structure : muets, ils feraient croire a
    // une garantie de frontiere qui n'est pas la.
    chkErr("BOUNDARY : END_BOUNDARY orphelin refuse",
           "  org #2F00\n  db 1\n  END_BOUNDARY\n");
    chkErr("BOUNDARY : un bloc jamais ferme est refuse",
           "  org #2F00\n  BOUNDARY 256\n  db 1\n");
    // L'imbrication est REFUSEE, et non silencieusement mal mesuree : la mesure
    // du bloc externe s'arreterait au premier END_BOUNDARY, celui du bloc
    // interne, et rendrait une garantie de frontiere fausse.
    chkErr("BOUNDARY : l'imbrication est refusee",
           "  org #2F00\n"
           "  BOUNDARY 256\n"
           "  BOUNDARY 16\n"
           "  db 1\n"
           "  END_BOUNDARY\n"
           "  db 2\n"
           "  END_BOUNDARY\n");

    // --- PUBLIC / EXTERN : la portee entre objets (§4.4) ---------------------
    printf("\n-- PUBLIC / EXTERN --\n");
    {
        // Un symbole est LOCAL par defaut ; `PUBLIC` l'exporte.
        Built o = build("  public start\n  section code,\"ro\"\nstart:\n  nop\nother:\n  ret\n", "t.asm");
        bool pub = false, loc = true;
        for (const auto &s : o.obj.symbolTable) {
            if (s.name == "start") pub = s.isPublic;
            if (s.name == "other" && s.isPublic) loc = false;
        }
        okc("PUBLIC exporte le nom", o.ok && pub);
        okc("et le defaut reste local", loc);
    }
    // `EXTERN` declare defini ailleurs : l'assemblage passe, c'est le LINKAGE qui
    // reclame une definition — et un seul objet n'en a aucune a offrir.
    {
        Built o = build("  extern helper\n  section code,\"ro\"\n  call helper\n", "t.asm");
        okc("un EXTERN assemble sans erreur d'assemblage", o.obj.ok);
        bool named = !o.errors.empty() &&
                     o.errors[0].message.find("helper") != std::string::npos &&
                     o.errors[0].message.find("EXTERN") != std::string::npos;
        okc("mais le linkage refuse en nommant le symbole", !o.ok && named);
        okc("et il ne le dit qu'une fois", o.errors.size() == 1);
    }
    // Le point de D10 : un nom ni defini ni EXTERN reste une erreur
    // d'ASSEMBLAGE, a sa ligne — et non une relocalisation non resolue signalee
    // deux maillons plus loin.
    {
        Built o = build("  section code,\"ro\"\n  call typo\n", "t.asm");
        bool here = !o.errors.empty() && o.errors[0].line == 2 &&
                    o.errors[0].message.find("typo") != std::string::npos;
        okc("un nom inconnu est refuse a SA ligne", !o.ok && here);
    }
    chkErr("PUBLIC d'un nom inconnu", "  public nowhere\n  nop\n");
    chkErr("PUBLIC d'un nom EXTERN", "  extern foo\n  public foo\n  nop\n");
    chkErr("EXTERN et defini ici", "  extern foo\nfoo:\n  nop\n");
    chkErr("l'ordre inverse est la meme faute", "foo:\n  nop\n  extern foo\n");
    chkErr("PUBLIC sans nom", "  public\n  nop\n");
    chkErr("EXTERN sans nom", "  extern\n  nop\n");
    // ADR 0015 : un mot reserve ne peut pas nommer un symbole.
    chkErr("un label ne peut pas s'appeler 'public'", "public:\n  nop\n");
    chkErr("ni 'high'", "high:\n  nop\n");
    chkErr("un EXTERN ne peut pas s'appeler comme un registre", "  extern a\n  nop\n");
    {
        // Deux EXTERN sur une ligne, et un `high()` sur l'un d'eux : le
        // mecanisme existe, meme si rien ne l'exerce encore (c'est B8).
        Built o = build("  extern alpha, beta\n  section c,\"ro\"\n  ld hl,alpha\n  ld d,high(beta)\n", "t.asm");
        okc("deux EXTERN se declarent d'une ligne", o.obj.ok);
        okc("chacun reclame sa definition", o.errors.size() == 2);
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
