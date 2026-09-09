// fo_test.cpp - Tests du fichier objet : ecrire, relire, reecrire
//
// L'aller-retour se teste par CHAINES, et c'est justement la raison de ne pas
// faire le format compact : un objet faux se lit a l'oeil, et un test qui
// echoue montre la ligne qui differe.
#include "fo.h"

#include <cstdio>
#include <string>

static int g_pass = 0, g_fail = 0;

static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

// Ecrire, relire, reecrire : les deux textes doivent etre identiques. C'est le
// seul controle qui attrape a la fois un champ qu'on n'ecrit pas et un champ
// qu'on ne relit pas.
static void roundTrip(const char *desc, const std::string &src) {
    const asmb::Object a = asmb::assembleText(src, "t.asm");
    if (!a.ok) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : le source ne s'assemble pas\n", desc);
        for (const auto &e : a.errors) printf("    %s:%d %s\n", e.file.c_str(), e.line, e.message.c_str());
        return;
    }
    const std::string first = fo::write(a);
    asmb::Object b;
    std::string err;
    if (!fo::read(first, b, err)) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : relecture refusee : %s\n", desc, err.c_str());
        return;
    }
    const std::string second = fo::write(b);
    if (first == second) { ++g_pass; return; }
    ++g_fail;
    printf("  \033[31mFAIL\033[0m %s : l'aller-retour n'est pas stable\n", desc);
    size_t p1 = 0, p2 = 0;
    while (p1 < first.size() && p2 < second.size()) {
        const size_t e1 = first.find('\n', p1), e2 = second.find('\n', p2);
        const std::string l1 = first.substr(p1, e1 - p1), l2 = second.substr(p2, e2 - p2);
        if (l1 != l2) { printf("    ecrit : %s\n    relu  : %s\n", l1.c_str(), l2.c_str()); break; }
        p1 = e1 + 1; p2 = e2 + 1;
    }
}

// Un `.fo` malforme est refuse, et le diagnostic NOMME la ligne fautive.
static void bad(const char *desc, const std::string &text, int line) {
    asmb::Object o;
    std::string err;
    if (fo::read(text, o, err)) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : aurait du etre refuse\n", desc);
        return;
    }
    const std::string want = "line " + std::to_string(line) + ":";
    if (err.compare(0, want.size(), want) != 0) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s : attendu « %s », obtenu « %s »\n", desc, want.c_str(), err.c_str());
        return;
    }
    ++g_pass;
}

static const char *kHead = "fantams-object 1\n";

int main() {
    printf("Tests fichier objet (.fo)\n");

    // --- L'aller-retour ------------------------------------------------------
    roundTrip("le plus simple", "  org #8000\n  nop\n");
    roundTrip("labels et symboles", "  org #8000\nstart:\n  ld hl,start\nSIZE equ 4\n  ret\n");
    roundTrip("un trou reserve", "  org #8000\n  db 1\n  ds 4\n  db 2\n");
    roundTrip("plusieurs org", "  org #8000\n  db 1\n  org #9000\n  db 2\n");
    roundTrip("un org deplace", "  org #2000,#3000\n  nop\n");
    roundTrip("un prefixe de banque", "  org b5:#4000\n  db #AB\n");
    roundTrip("des sections absolues",
              "  section code,\"ro\"\n  org #8000\n  ld a,1\n  section data,\"rw\",8\n  org #9000\n  db 1,2\n");
    roundTrip("une section relocalisable",
              "  section code,\"ro\"\nstart:\n  ld hl,tbl\n  jr start\n  section data,\"rw\"\ntbl:\n  dw tbl\n");
    roundTrip("les quatre relocalisations",
              "  section code,\"ro\"\n  ld hl,tbl\n  ld a,high(tbl)\n  ld b,low(tbl)\n"
              "  section data,\"rw\"\ntbl:\n  dw tbl\n");
    roundTrip("PUBLIC et EXTERN",
              "  public start\n  extern helper\n  section code,\"ro\"\nstart:\n  call helper\n");
    roundTrip("un point d'entree nomme", "  org #8000\nmain:\n  ret\n  run main\n");
    roundTrip("un point d'entree litteral", "  org #8000\n  ret\n  run #8000\n");
    roundTrip("une section uninit", "  section bss,\"uninit\",16\nbuf:\n  ds 8\n");
    roundTrip("plus de seize octets par ligne",
              "  org #8000\n  db 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20\n");
    roundTrip("un chemin a espace et virgule", "  org #8000\n  nop\n");
    roundTrip("des acces a adresse litterale",
              "  section code,\"ro\"\n  ld (tbl),a\n  ld (#C000),hl\n"
              "  section data,\"rw\"\ntbl:\n  db 0\n");

    // --- Ce que le texte doit contenir, et qu'un humain doit y lire ----------
    {
        const asmb::Object a = asmb::assembleText("  org #8000\nstart:\n  ld hl,start\n", "t.asm");
        const std::string t = fo::write(a);
        ok("l'en-tete nomme le format", t.compare(0, 15, "fantams-object ") == 0);
        ok("les octets sont en hexadecimal a deux chiffres", t.find("21 00 80") != std::string::npos);
        ok("un symbole s'y lit avec sa provenance",
           t.find("symbol \"start\" label local") != std::string::npos &&
           t.find("file=\"t.asm\" line=2") != std::string::npos);
    }
    {
        // La table de `--sym` en est un SOUS-ENSEMBLE : memes noms, plus la
        // portee et l'ancrage dans un fragment.
        const asmb::Object a = asmb::assembleText("  public start\n  org #8000\nstart:\n  ret\n", "t.asm");
        const std::string t = fo::write(a);
        ok("le bloc symbol porte la portee", t.find("symbol \"start\" label public") != std::string::npos);
        ok("et l'ancrage dans un fragment", t.find("frag=0 offset=0x0") != std::string::npos);
    }
    {
        // Un trou reserve se lit comme tel, et n'est pas un octet a zero.
        const asmb::Object a = asmb::assembleText("  section b,\"uninit\"\n  ds 4\n", "t.asm");
        const std::string t = fo::write(a);
        ok("une section uninit n'a pas d'octets", t.find("data ") == std::string::npos);
    }

    // --- Un fichier malforme est refuse, en nommant sa ligne -----------------
    printf("\n  refus\n");
    bad("un fichier vide", "", 0);
    bad("pas un objet fantams", "hello\n", 1);
    bad("une version inconnue", "fantams-object 99\n", 1);
    bad("une version illisible", "fantams-object x\n", 1);
    bad("un bloc inconnu", std::string(kHead) + "widget 3\n", 2);
    bad("un fragment non ferme", std::string(kHead) + "fragment 0 addr=0x0\n", 2);
    bad("un fragment hors d'ordre", std::string(kHead) + "fragment 7 addr=0x0\nend\n", 2);
    bad("un site hors d'ordre", std::string(kHead) + "site 3 \"a.asm\" 1\n", 2);
    bad("un octet qui n'en est pas un",
        std::string(kHead) + "site 0 \"a.asm\" 1\nfragment 0 addr=0x0\n  data 0 ZZ\nend\n", 4);
    bad("un octet a un seul chiffre",
        std::string(kHead) + "site 0 \"a.asm\" 1\nfragment 0 addr=0x0\n  data 0 F\nend\n", 4);
    bad("un gap qui n'est pas un nombre",
        std::string(kHead) + "fragment 0 addr=0x0\n  gap x\nend\n", 3);
    bad("une ligne inconnue dans un fragment",
        std::string(kHead) + "fragment 0 addr=0x0\n  widget\nend\n", 3);
    bad("une relocalisation sans type",
        std::string(kHead) + "reloc frag=0 offset=0x0 section=0 addend=0x0\n", 2);
    bad("un symbole sans type", std::string(kHead) + "symbol \"x\" value=0x1\n", 2);
    bad("une chaine non terminee", std::string(kHead) + "site 0 \"a.asm\n", 2);
    bad("une taille de section illisible",
        std::string(kHead) + "section \"a\" id=0 abs size=oups\n", 2);

    // --- Les acces a adresse litterale (§4.6, bloc 4) ------------------------
    {
        const asmb::Object a = asmb::assembleText(
            "  org #8000\n  ld (#C000),a\n  ld hl,#C000\n  ld (hl),a\n", "t.asm");
        ok("une ecriture a adresse litterale est consignee", a.accesses.size() == 1);
        ok("avec son fragment, son offset et son sens",
           a.accesses.size() == 1 && a.accesses[0].frag == 0 &&
           a.accesses[0].offset == 1 && a.accesses[0].kind == asmb::Access::MemWrite &&
           a.accesses[0].addend == 0xC000);
        // La limite est ECRITE, pas a decouvrir : `ld (hl),a` n'y figure pas et
        // ne sera jamais attrape.
        ok("un acces dont l'adresse est calculee n'y figure pas", a.accesses.size() == 1);
    }
    {
        // Une LECTURE n'y figure pas : l'etage B ne consigne que les ecritures,
        // le sens que le controle en "ro" produit deja.
        const asmb::Object a = asmb::assembleText("  org #8000\n  ld a,(#C000)\n", "t.asm");
        ok("une lecture n'y figure pas (C2 s'en chargera)", a.accesses.empty());
    }
    {
        // Vers une section relocalisable, l'acces dit ce que le linker dira :
        // une base de section plus un decalage.
        const asmb::Object a = asmb::assembleText(
            "  section code,\"ro\"\n  ld (tbl+2),a\n  section data,\"rw\"\ntbl:\n  ds 4\n", "t.asm");
        ok("il cite la section visee et son decalage",
           a.accesses.size() == 1 && a.accesses[0].section >= 0 && a.accesses[0].addend == 2);
    }
    {
        const asmb::Object a = asmb::assembleText(
            "  extern port\n  section code,\"ro\"\n  ld (port),a\n", "t.asm");
        ok("ou le symbole EXTERN vise",
           a.accesses.size() == 1 && a.accesses[0].symbol == "port");
        const std::string t = fo::write(a);
        ok("et le bloc se lit dans le texte",
           t.find("access frag=0 offset=0x1 write symbol=\"port\"") != std::string::npos);
    }

    // --- Un objet relu est le meme objet -------------------------------------
    printf("\n  fidelite\n");
    {
        const asmb::Object a = asmb::assembleText(
            "  public start\n  section code,\"ro\"\nstart:\n  ld hl,tbl\n  jr start\n"
            "  section data,\"rw\",8\ntbl:\n  dw tbl\n  ds 3\n  run start\n", "t.asm");
        asmb::Object b;
        std::string err;
        ok("il se relit", fo::read(fo::write(a), b, err));
        ok("les fragments sont tous la", a.fragments.size() == b.fragments.size());
        ok("les relocalisations aussi", a.relocs.size() == b.relocs.size());
        ok("les acces litteraux aussi", a.accesses.size() == b.accesses.size());
        ok("les sites aussi", a.sites.size() == b.sites.size());
        ok("les sections aussi", a.sections.size() == b.sections.size());
        ok("les symboles aussi", a.symbolTable.size() == b.symbolTable.size());
        ok("le point d'entree survit",
           b.entry.has == a.entry.has && b.entry.name == a.entry.name);
        bool bytes = a.fragments.size() == b.fragments.size();
        for (size_t i = 0; bytes && i < a.fragments.size(); ++i)
            bytes = a.fragments[i].bytes == b.fragments[i].bytes &&
                    a.fragments[i].prov == b.fragments[i].prov &&
                    a.fragments[i].addr == b.fragments[i].addr &&
                    a.fragments[i].logical == b.fragments[i].logical &&
                    a.fragments[i].bank == b.fragments[i].bank &&
                    a.fragments[i].placed == b.fragments[i].placed &&
                    a.fragments[i].relocSection == b.fragments[i].relocSection &&
                    a.fragments[i].section == b.fragments[i].section;
        ok("les octets et leur provenance sont intacts", bytes);
        ok("`symbols` est reconstruite depuis la table exportable",
           b.symbols.count("start") != 0 && b.symbols.count("tbl") != 0);
    }

    {
        // P2 : le placement porte par la declaration de section fait l'aller et
        // le retour. C'est la propriete de cette suite — ecrire, relire,
        // reecrire rend le meme texte —, appliquee aux deux nouvelles cles.
        asmb::Object o = asmb::assembleText(
            "  section gfx1, \"ro\" IN ext_w1<1>\n  db 1\n"
            "  section gfx2, \"ro\" IN w1 OF all_ext\n  db 2\n"
            "  section code, \"ro\"\n  db 3\n", "t.asm");
        ok("la source s'assemble", o.ok);
        const std::string text = fo::write(o);
        ok("la forme courte s'ecrit", text.find("place=\"ext_w1<1>\"") != std::string::npos);
        ok("la forme verbeuse ecrit ses deux cles",
            text.find("place=\"all_ext\"") != std::string::npos &&
            text.find("window=\"w1\"") != std::string::npos);
        ok("une section sans placement n'ecrit ni l'une ni l'autre",
            text.find("\"code\" id=2 type=RO reloc size=0x1 at=") != std::string::npos);
        std::string err;
        asmb::Object back;
        const bool read = fo::read(text, back, err);
        ok("et la relecture le rend", read && back.sections.size() == 3 &&
            back.sections[0].place == "ext_w1<1>" && back.sections[0].placeWindow.empty() &&
            back.sections[1].place == "all_ext" && back.sections[1].placeWindow == "w1" &&
            back.sections[2].place.empty());
        ok("ecrire, relire, reecrire rend le meme texte", fo::write(back) == text);
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
