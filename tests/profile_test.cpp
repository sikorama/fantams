// profile_test.cpp - Tests de l'analyseur de profil de cible
//
// Deux choses s'y verifient, et la seconde est le test d'acceptation du modele
// que le §13.2 demande « a C1 » :
//
//  1. la grammaire — un texte entre, une valeur sort, sans un octet ;
//  2. **que decrire une machine ne demande pas de toucher au code du linker.**
//     Les blocs ZX et MSX de cette suite ne sont pas livres : ils sont la pour
//     exercer le vocabulaire, et ce sont eux qui prouvent que les fenetres, les
//     tailles de banque et les axes ne sont pas cables sur une seule machine.
#include "profile.h"

#include <cstdio>
#include <string>

static int g_pass = 0, g_fail = 0;

static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

static profile::Profile parse(const std::string &text) {
    return profile::parse(text, "machine.prof");
}

static bool says(const profile::Profile &p, const char *needle) {
    for (const asmb::Diagnostic &d : p.errors)
        if (d.message.find(needle) != std::string::npos) return true;
    return false;
}

static void firstError(const profile::Profile &p) {
    if (!p.errors.empty())
        printf("    premiere erreur : %s:%d: %s\n", p.errors[0].file.c_str(),
               p.errors[0].line, p.errors[0].message.c_str());
}

static const profile::Window *window(const profile::Profile &p, const char *n) {
    for (const profile::Window &w : p.windows) if (w.name == n) return &w;
    return nullptr;
}
static const profile::Bank *bank(const profile::Profile &p, const char *n) {
    for (const profile::Bank &b : p.banks) if (b.name == n) return &b;
    return nullptr;
}
static const profile::Axis *axis(const profile::Profile &p, const char *n) {
    for (const profile::Axis &a : p.axes) if (a.name == n) return &a;
    return nullptr;
}
static const profile::State *state(const profile::Axis &a, const char *n) {
    for (const profile::State &s : a.states) if (s.name == n) return &s;
    return nullptr;
}

// Un profil minimal, valide, auquel les tests ajoutent ce qu'ils exercent : sans
// lui chaque cas devrait reecrire une fenetre, une banque et un SELECT.
static std::string with(const std::string &more) {
    return "WINDOW w0 [0x0000..0x3FFF]\n"
           "BANK b0 SIZE 0x4000 rw STORE 99\n" + more;   // 99 : hors de portee
                                                        // des STORE que les cas ajoutent
}

int main() {
    printf("Tests profil de cible (texte -> Profile)\n");

    // --- Le profil LIVRE se lit, et il est le seul porteur de ses valeurs ---
    {
        // Le test d'acceptation n°2 du §13.2 : charger chaque profil livre et
        // verifier qu'il se lit, sans qu'aucun code de linker connaisse son nom.
        // La boucle est sur `builtinNames()` justement pour cela.
        const std::vector<std::string> names = profile::builtinNames();
        ok("au moins un profil est livre", !names.empty());
        for (const std::string &n : names) {
            const std::string text = profile::builtin(n);
            ok("le profil livre porte du texte", !text.empty());
            profile::Profile p = profile::parse(text, n);
            ok("le profil livre se lit sans une erreur", p.ok);
            if (!p.ok) firstError(p);
            ok("il nomme sa machine", p.hasTarget && !p.target.empty());
            ok("il declare des fenetres, des banques et des axes",
               !p.windows.empty() && !p.banks.empty() && !p.axes.empty());
            // Les citations FONT PARTIE du profil : un profil sans elles est un
            // profil que personne ne peut auditer (§12.3).
            ok("il distingue ce qui est atteste de ce qui n'est pas tranche",
               text.find("ATTESTE") != std::string::npos &&
               text.find("NON TRANCHE") != std::string::npos);
        }
        ok("un nom inconnu ne rend aucun texte", profile::builtin("pas_une_machine").empty());
    }

    // --- Les fenetres -------------------------------------------------------
    {
        profile::Profile p = parse("WINDOW w1 [0x4000..0x7FFF]\n");
        ok("une fenetre est une plage", p.ok && p.windows.size() == 1 &&
                                        p.windows[0].lo == 0x4000 && p.windows[0].hi == 0x7FFF);
    }
    {
        profile::Profile p = parse("WINDOW w1 [0x4000..0x7FFF]\nWINDOW w1 [0x8000..0xBFFF]\n");
        ok("deux fenetres du meme nom sont refusees", !p.ok);
        ok("et le refus nomme la premiere ligne", says(p, "declared twice") && says(p, "line 1"));
    }
    {
        profile::Profile p = parse("WINDOW w1 [0x4000]\n");
        ok("une fenetre qui n'est pas une plage est refusee", !p.ok);
        ok("et le refus donne la forme", says(p, "[0x4000..0x7FFF]"));
    }

    // --- Les banques : la taille est DECLAREE, jamais implicite -------------
    {
        profile::Profile p = parse("WINDOW w0 [0..0x3FFF]\nBANK seg0 rw STORE 0\n");
        ok("une banque sans SIZE est refusee", !p.ok);
        ok("et le refus dit pourquoi la taille ne peut pas etre implicite",
           says(p, "declared, never implicit") && says(p, "indescribable"));
    }
    {
        // Une plage de banques declare N banques, et non une banque de N x 16 K.
        profile::Profile p = parse(with("BANK base0..base3 SIZE 0x4000 rw VIDEO STORE 0..3\n"));
        ok("une plage declare chaque banque", p.ok && bank(p, "base0") && bank(p, "base3"));
        ok("et pas celles d'a cote", !bank(p, "base4"));
        ok("les attributs vont a toutes",
           bank(p, "base2") && bank(p, "base2")->video && !bank(p, "base2")->readOnly &&
           bank(p, "base2")->size == 0x4000);
    }
    {
        // Une ligne SANS taille AMENDE : c'est ainsi que la contention se pose
        // sur quatre banques d'un lot de huit sans repeter leur taille. Et c'est
        // exactement ce qu'un ZX exige.
        profile::Profile p = parse(with("BANK ram0..ram7 SIZE 0x4000 rw STORE 0..7\n"
                                        "BANK ram1, ram3, ram5, ram7 CONTENDED\n"
                                        "BANK ram5, ram7 VIDEO\n"
                                        "CONFIG SET pager { high<n> [CODE n] { w0 ram<n> } }\n"
                                        "SELECT pager = OUT 0x7FFD, MASK %00000111, CODE\n"));
        ok("un profil ZX en miniature se lit", p.ok);
        if (!p.ok) firstError(p);
        ok("la contention frappe les banques nommees, et elles seules",
           bank(p, "ram1") && bank(p, "ram1")->contended &&
           bank(p, "ram2") && !bank(p, "ram2")->contended);
        ok("un amendement ne perd pas la taille",
           bank(p, "ram7") && bank(p, "ram7")->hasSize && bank(p, "ram7")->size == 0x4000 &&
           bank(p, "ram7")->contended && bank(p, "ram7")->video);
    }
    {
        profile::Profile p = parse(with("BANK b1 SIZE 0x4000 ro rw STORE 1\n"));
        ok("ro et rw ensemble sont refuses", !p.ok);
    }

    // --- STORE : l'emplacement de rangement est DECLARE ---------------------
    {
        profile::Profile p = parse("WINDOW w0 [0..0x3FFF]\nBANK b0 SIZE 0x4000 rw\n");
        ok("une banque sans STORE est refusee", !p.ok);
        ok("et le refus dit pourquoi", says(p, "storage slot is unnamed"));
    }
    {
        // Une plage de banques recoit une plage de numeros, un pour un. Un seul
        // numero pour quatre banques aurait ete une attribution consecutive
        // IMPLICITE, et l'implicite est ce que STORE existe pour retirer.
        profile::Profile p = parse(with("BANK e0..e3 SIZE 0x4000 rw STORE 4..7\n"));
        ok("une plage de banques recoit une plage de numeros", p.ok);
        if (!p.ok) firstError(p);
        ok("un pour un, dans l'ordre",
           bank(p, "e0") && bank(p, "e0")->store == 4 &&
           bank(p, "e3") && bank(p, "e3")->store == 7);
    }
    {
        profile::Profile p = parse(with("BANK e0..e3 SIZE 0x4000 rw STORE 4\n"));
        ok("un seul numero pour quatre banques est refuse", !p.ok);
        ok("et le refus donne la forme", says(p, "STORE a..b"));
    }
    {
        profile::Profile p = parse(with("BANK e0 SIZE 0x4000 rw STORE 5\n"
                                        "BANK e1 SIZE 0x4000 rw STORE 5\n"));
        ok("deux banques dans le meme emplacement sont refusees", !p.ok);
        ok("et le refus nomme les deux", says(p, "'e1' and 'e0' both declare STORE 5"));
    }
    {
        // Un amendement peut porter le STORE seul, comme il porte CONTENDED.
        profile::Profile p = parse(with("BANK e0..e1 SIZE 0x4000 rw STORE 4..5\n"
                                        "BANK e1 CONTENDED\n"));
        ok("un amendement ne perd pas le STORE",
           p.ok && bank(p, "e1") && bank(p, "e1")->store == 5 && bank(p, "e1")->contended);
        if (!p.ok) firstError(p);
    }

    // --- Deux grilles superposees, et des banques de 8 K --------------------
    {
        // Les deux exigences du §13.1, et celles qui ont fait tomber le 16 K
        // cable. Un MSX en miniature : quatre pages de slot de 16 K, quatre
        // fenetres de mapper de 8 K superposees, et des banques de 8 K.
        profile::Profile p = parse(
            "WINDOW page1 [0x4000..0x7FFF]\n"
            "WINDOW page2 [0x8000..0xBFFF]\n"
            "WINDOW m0 [0x4000..0x5FFF]\n"
            "WINDOW m1 [0x6000..0x7FFF]\n"
            "BANK seg0..seg3 SIZE 0x2000 ro STORE 0..3\n"
            "CONFIG SET mapper1 { seg<n> [CODE n] { m1 seg<n> } }\n"
            "SELECT mapper1 = POKE 0x6000, CODE\n");
        ok("un MSX en miniature se lit", p.ok);
        if (!p.ok) firstError(p);
        ok("deux grilles se recouvrent sans que rien ne s'en plaigne",
           window(p, "page1") && window(p, "m0") &&
           window(p, "m0")->lo == window(p, "page1")->lo);
        ok("une banque de 8 K est une banque comme une autre",
           bank(p, "seg0") && bank(p, "seg0")->size == 0x2000);
        ok("la commutation peut etre une ECRITURE MEMOIRE, pas seulement un OUT",
           p.selects.size() == 1 && p.selects[0].writes.size() == 1 &&
           p.selects[0].writes[0].kind == profile::Write::Poke);
    }

    // --- Les configurations -------------------------------------------------
    {
        profile::Profile p = parse(
            "WINDOW w0 [0..0x3FFF]\nWINDOW w1 [0x4000..0x7FFF]\n"
            "BANK base0..base1 SIZE 0x4000 rw STORE 0..1\nBANK ext0..ext3 SIZE 0x4000 rw STORE 4..7\n"
            "CONFIG SET ram {\n"
            "  linear    [CODE %000]     { w0 base0  w1 base1  }\n"
            "  ext_w1<b> [CODE %100 | b] { w0 base0  w1 ext<b> }\n"
            "}\n"
            "SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE\n");
        ok("un axe a deux etats se lit", p.ok);
        if (!p.ok) firstError(p);
        ok("l'etat parametrique porte son parametre",
           axis(p, "ram") && state(*axis(p, "ram"), "ext_w1") &&
           state(*axis(p, "ram"), "ext_w1")->hasParam &&
           state(*axis(p, "ram"), "ext_w1")->param == "b");
        ok("son CODE est une EXPRESSION, non un nombre",
           axis(p, "ram") && state(*axis(p, "ram"), "ext_w1")->hasCode &&
           state(*axis(p, "ram"), "ext_w1")->code.kind == profile::Expr::Binary &&
           state(*axis(p, "ram"), "ext_w1")->code.op == "|");
        ok("la banque d'un slot peut etre parametrique",
           axis(p, "ram") && state(*axis(p, "ram"), "ext_w1")->slots.size() == 2 &&
           state(*axis(p, "ram"), "ext_w1")->slots[1].bank == "ext" &&
           state(*axis(p, "ram"), "ext_w1")->slots[1].hasParam);
        ok("la valeur du SELECT est un arbre, non evalue",
           p.selects.size() == 1 && p.selects[0].writes.size() == 1 &&
           p.selects[0].writes[0].value.kind == profile::Expr::Binary);
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { w0 pas_une_banque } }\n"
                                        "SELECT a = OUT 0, 0\n"));
        ok("une configuration nommant une banque inconnue est refusee", !p.ok);
        ok("et le refus la nomme", says(p, "'pas_une_banque' is not a declared BANK"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { pas_une_fenetre b0 } }\n"
                                        "SELECT a = OUT 0, 0\n"));
        ok("une configuration nommant une fenetre inconnue est refusee", !p.ok);
        ok("et le refus la nomme", says(p, "not a declared WINDOW"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { w0 b0 }  s { w0 b0 } }\n"
                                        "SELECT a = OUT 0, 0\n"));
        ok("deux etats du meme nom sont refuses", !p.ok);
        ok("et le refus le dit", says(p, "declared twice"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { w0 b0 } }\n"));
        ok("un axe sans SELECT est refuse", !p.ok);
        ok("et le refus dit pourquoi",
           says(p, "has no SELECT") && says(p, "never show"));
    }
    {
        profile::Profile p = parse(with("SELECT pas_un_axe = OUT 0, 0\n"));
        ok("un SELECT sur un axe inconnu est refuse", !p.ok);
        ok("et le refus le nomme", says(p, "names no declared axis"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { w0 b0 } }\n"
                                        "SELECT a = 0x7F00, 0\n"));
        ok("un SELECT sans OUT ni POKE est refuse", !p.ok);
        ok("et le refus dit ce qu'est une commutation",
           says(p, "sequence of writes"));
    }
    {
        // `OVER` declare qu'un axe recouvre un autre. Le lire a l'envers ferait
        // declarer conforme un octet ecrit dans le vide (§13.1) : l'axe recouvert
        // doit donc etre declare AVANT.
        profile::Profile p = parse(with("CONFIG SET rom OVER ram { on { w0 b0 } }\n"
                                        "SELECT rom = OUT 0, 0\n"));
        ok("un OVER sur un axe non declare est refuse", !p.ok);
        ok("et le refus dit qu'il doit venir avant", says(p, "declared before it"));
    }

    // --- Ce qui est reconnu et refuse en nommant l'etage --------------------
    {
        profile::Profile p = parse(with("PAGING LOCKS ON 0x7FFD BIT 5\n"));
        ok("PAGING est reconnu et refuse", !p.ok);
        ok("et le refus nomme C2", says(p, "stage C2"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { s { w0 b0 } }\n"
                                        "SELECT a STACK OUTSIDE [0..0xFFFF]\n"));
        ok("une contrainte de pile est reconnue et refusee", !p.ok);
        ok("et le refus nomme C2", says(p, "stage C2"));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { MIRROR [b0..b0] AT OFFSET 0 { SECTION s } }\n"
                                        "SELECT a = OUT 0, 0\n"));
        ok("MIRROR est reconnu et refuse", !p.ok);
        ok("et le refus nomme C2 et la continuite",
           says(p, "C2") && says(p, "continuity"));
    }

    // --- SHADOWS et ALWAYS ne sont pas des mots du langage ------------------
    {
        profile::Profile p = parse(with("SHADOWS w1\n"));
        ok("SHADOWS est refuse", !p.ok);
        ok("et le refus dit qu'il se CALCULE",
           says(p, "CALCULATED from the configurations"));
    }
    {
        profile::Profile p = parse(with("ALWAYS w0\n"));
        ok("ALWAYS est refuse", !p.ok);
        ok("et par la meme raison", says(p, "CALCULATED"));
    }

    // --- Le ternaire, et le masque qui l'a remplace -------------------------
    {
        // La forme `RMR.BIT2 = (on ? 0 : 1)` du §6 disait un MASQUE avec le nom
        // d'un registre CPC dans la grammaire. Le masque est maintenant explicite,
        // et le ternaire refuse en le disant.
        profile::Profile p = parse(with("CONFIG SET a { on { w0 b0 } }\n"
                                        "SELECT a = OUT 0x7F00, (on ? 0 : 1)\n"));
        ok("un ternaire dans une valeur est refuse", !p.ok);
        ok("et le refus renvoie au MASK et au CODE de l'etat",
           says(p, "MASK") && says(p, "[CODE ..."));
    }
    {
        profile::Profile p = parse(with("CONFIG SET a { on [CODE 0] { w0 b0 } }\n"
                                        "SELECT a = OUT 0x7F00, MASK %00000100, CODE << 2\n"));
        ok("la forme avec MASK se lit", p.ok);
        if (!p.ok) firstError(p);
        ok("et le masque est porte",
           p.selects.size() == 1 && p.selects[0].writes[0].hasMask &&
           p.selects[0].writes[0].mask.num == 4);
    }
    {
        // Deux ecritures pour un axe : c'est ce que le §12.3 decrit pour la ROM
        // haute, dont la seconde ecriture porte le numero.
        profile::Profile p = parse(with("CONFIG SET a { on [CODE 0] { w0 b0 } }\n"
                                        "SELECT a = OUT 0x7F00, MASK %00001000, CODE << 3\n"
                                        "            OUT 0xDF00, MASK %11111111, PAGE\n"));
        ok("un axe peut demander deux ecritures", p.ok);
        if (!p.ok) firstError(p);
        ok("et les deux sont portees, dans l'ordre",
           p.selects.size() == 1 && p.selects[0].writes.size() == 2 &&
           p.selects[0].writes[1].port.num == 0xDF00 &&
           p.selects[0].writes[1].value.kind == profile::Expr::Name &&
           p.selects[0].writes[1].value.name == "PAGE");
    }

    // --- Le reste du lexique ------------------------------------------------
    {
        profile::Profile p = parse("BUILDSNA\n");
        ok("un mot inconnu est refuse", !p.ok);
        ok("le refus nomme les cinq mots d'un profil",
           says(p, "TARGET, WINDOW, BANK, CONFIG SET and SELECT"));
    }
    {
        profile::Profile p = parse("TARGET a\nTARGET b\n");
        ok("deux TARGET sont refuses", !p.ok);
        ok("et le refus dit qu'un profil decrit une machine", says(p, "one machine"));
    }
    {
        profile::Profile p = parse("");
        ok("un profil vide est licite et ne declare rien",
           p.ok && p.windows.empty() && p.banks.empty() && p.axes.empty());
    }
    {
        profile::Profile p = parse(with("CONFIG a { s { w0 b0 } }\n"));
        ok("un CONFIG sans SET est refuse", !p.ok);
        ok("et le refus dit qui declare et qui nomme",
           says(p, "a profile declares axes") && says(p, "a script names their states"));
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
