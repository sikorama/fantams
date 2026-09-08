// script_test.cpp - Tests de l'analyseur de script de linkage
//
// Le gain de test de cette suite est qu'elle part d'un TEXTE et n'en sort
// jamais : pas de profil, pas d'objet, pas un octet. Une faute de syntaxe
// nomme l'analyseur, et rien d'autre.
#include "script.h"

#include <cstdio>
#include <string>

static int g_pass = 0, g_fail = 0;

static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

static script::Script parse(const std::string &text) {
    return script::parse(text, "game.ld");
}

// Le premier message, ou une chaine vide : un test qui deref
// `errors[0]` sur un script accepte par erreur ferait tomber la suite entiere
// au lieu de nommer sa ligne.
static std::string firstError(const script::Script &s) {
    return s.errors.empty() ? std::string() : s.errors[0].message;
}

static bool says(const script::Script &s, const char *needle) {
    for (const asmb::Diagnostic &d : s.errors)
        if (d.message.find(needle) != std::string::npos) return true;
    return false;
}

int main() {
    printf("Tests script de linkage (texte -> Script)\n");

    // --- Le script du §12.2, en entier --------------------------------------
    {
        // Dix lignes pour un programme banque reel : c'est l'affirmation du §6,
        // et ce test la tient.
        script::Script s = parse(
            "TARGET cpc6128 + RAM128\n"
            "\n"
            "MEMORY_MAP {\n"
            "    CONFIG linear {\n"
            "        w1 { SECTION main     }\n"
            "        w2 { SECTION sysbank  }\n"
            "        w3 { SECTION unpacked }\n"
            "    }\n"
            "    CONFIG ext_w1<1> { w1 { SECTION audio } }\n"
            "}\n"
            "\n"
            "OUTPUT_FORMAT {\n"
            "    CONTAINER   = \"SNA_V2\"\n"
            "    ENTRY_POINT = 0x8000\n"
            "    STACK       = [0x3F00..0x3FFF]\n"
            "    INT_VECTOR  = 0x0038\n"
            "}\n");
        ok("le script du §12.2 est accepte", s.ok);
        if (!s.ok) printf("    premiere erreur : %s\n", firstError(s).c_str());
        ok("la machine et son extension", s.hasTarget && s.target == "cpc6128" &&
                                          s.extensions == std::vector<std::string>{"RAM128"});
        ok("deux configurations", s.map.size() == 2);
        ok("la premiere nomme un etat sans son axe",
           s.map.size() == 2 && s.map[0].config.axis.empty() &&
           s.map[0].config.state == "linear" && !s.map[0].config.hasArg);
        ok("trois fenetres dans la premiere", s.map.size() == 2 && s.map[0].placements.size() == 3);
        ok("chaque fenetre porte son numero et sa section",
           s.map.size() == 2 && s.map[0].placements.size() == 3 &&
           s.map[0].placements[0].window == 1 &&
           s.map[0].placements[0].sections == std::vector<std::string>{"main"} &&
           s.map[0].placements[2].window == 3 &&
           s.map[0].placements[2].sections == std::vector<std::string>{"unpacked"});
        ok("un etat parametrique porte son argument",
           s.map.size() == 2 && s.map[1].config.state == "ext_w1" &&
           s.map[1].config.hasArg && s.map[1].config.arg == 1);
        ok("le conteneur, le point d'entree et le vecteur",
           s.output.hasContainer && s.output.container == "SNA_V2" &&
           s.output.hasEntry && s.output.entry == 0x8000 &&
           s.output.hasIntVector && s.output.intVector == 0x0038);
        ok("la pile est une PLAGE",
           s.output.hasStack && s.output.stackLo == 0x3F00 && s.output.stackHi == 0x3FFF);
    }

    // --- Le decoupage d'une banque, et l'axe nomme --------------------------
    {
        // `OFFSET` / `SIZE` est un decoupage de placement A L'INTERIEUR d'une
        // banque, et non une banque de 8 K (§13.1). L'analyseur le porte ; c'est
        // C1.6 qui l'emploie.
        script::Script s = parse(
            "MEMORY_MAP {\n"
            "    CONFIG rom_upper.on, ROM 15 {\n"
            "        w3 [OFFSET 0x0000, SIZE 0x2000] { SECTION audio_rom }\n"
            "        w3 [OFFSET 0x2000, SIZE 0x2000] { SECTION graphics_data\n"
            "                                          SECTION menu_text     }\n"
            "    }\n"
            "}\n");
        ok("le decoupage est accepte", s.ok);
        if (!s.ok) printf("    premiere erreur : %s\n", firstError(s).c_str());
        ok("l'axe et l'etat sont nommes tous les deux",
           s.map.size() == 1 && s.map[0].config.axis == "rom_upper" &&
           s.map[0].config.state == "on");
        ok("le qualificatif porte son nom et sa valeur",
           s.map.size() == 1 && s.map[0].qualifiers.size() == 1 &&
           s.map[0].qualifiers[0].name == "ROM" &&
           s.map[0].qualifiers[0].hasValue && s.map[0].qualifiers[0].value == 15);
        ok("les deux blocs de 8 K sont distincts",
           s.map.size() == 1 && s.map[0].placements.size() == 2 &&
           s.map[0].placements[0].hasRange && s.map[0].placements[0].offset == 0 &&
           s.map[0].placements[0].size == 0x2000 &&
           s.map[0].placements[1].offset == 0x2000);
        ok("deux sections dans un bloc gardent l'ordre du script",
           s.map.size() == 1 && s.map[0].placements.size() == 2 &&
           s.map[0].placements[1].sections ==
               std::vector<std::string>{"graphics_data", "menu_text"});
        ok("chaque placement porte sa ligne",
           s.map.size() == 1 && s.map[0].placements.size() == 2 &&
           s.map[0].placements[0].line == 3 && s.map[0].placements[1].line == 4);
    }

    // --- Aucune resolution : c'est l'affaire de C1.4 -------------------------
    {
        // Une configuration, une fenetre et une section qu'aucun profil ne porte
        // passent l'analyse SANS UN MOT. Le profil n'existe pas ici, et melanger
        // les deux couches rendrait l'analyseur intestable seul.
        script::Script s = parse("MEMORY_MAP { CONFIG cette_config_n_existe_pas {\n"
                                 "  w42 { SECTION jamais_declaree } } }\n");
        ok("l'analyseur ne resout rien, et ne s'en plaint pas", s.ok);
        ok("il porte quand meme ce qu'il a lu",
           s.map.size() == 1 && s.map[0].placements.size() == 1 &&
           s.map[0].placements[0].window == 42);
    }

    // --- Un mot inconnu est une ERREUR, jamais un silence -------------------
    {
        script::Script s = parse("BUILDSNA \"game.sna\"\n");
        ok("un mot inconnu est refuse", !s.ok);
        ok("le refus nomme les trois blocs d'un script",
           says(s, "TARGET, MEMORY_MAP and OUTPUT_FORMAT"));
        ok("et il nomme sa ligne", !s.errors.empty() && s.errors[0].line == 1);
    }
    {
        script::Script s = parse("OUTPUT_FORMAT { SETCRTC = 1 }\n");
        ok("une cle de sortie inconnue est refusee", !s.ok);
        ok("le refus liste les cles connues", says(s, "ENTRY_POINT"));
    }
    {
        script::Script s = parse("MEMORY_MAP { CONFIG linear { SECTION main } }\n");
        ok("une section hors d'une fenetre est refusee", !s.ok);
        ok("le refus nomme ce qu'on attendait", says(s, "w1"));
    }

    // --- Ce qui est reconnu, et refuse --------------------------------------
    {
        // COMPRESS n'est pas un mot inconnu : c'est un mot dont l'algorithme
        // n'est pas de cet etage. Le refus le dit, et nomme le repli du §8.
        script::Script s = parse(
            "MEMORY_MAP { CONFIG ext_w1<0> { w1 { SECTION music_lz  COMPRESS \"lz48\" } } }\n");
        ok("COMPRESS est reconnu et refuse", !s.ok);
        ok("le refus nomme l'enveloppe du §8, pas un etage futur",
           says(s, "declare an envelope") && says(s, "\"ro\""));
    }
    {
        script::Script s = parse(
            "MEMORY_MAP { CONFIG linear { MIRROR [ext0..ext3] AT OFFSET 0 { SECTION stub } } }\n");
        ok("MIRROR est reconnu et refuse", !s.ok);
        ok("le refus nomme l'etage C2 et la raison", says(s, "C2") && says(s, "continuity"));
    }

    // --- La pile est une plage, et le refus le dit --------------------------
    {
        script::Script s = parse("OUTPUT_FORMAT { STACK = 0x3FFF }\n");
        ok("une pile ecrite comme une adresse est refusee", !s.ok);
        ok("le refus donne la forme et la raison",
           says(s, "[0x3F00..0x3FFF]") && says(s, "SP moves"));
    }
    {
        // La seconde forme fautive — un crochet, mais une seule borne — recoit le
        // MEME refus : c'est la meme faute, et un « expected '..' » dirait la
        // syntaxe sans dire la raison, la ou la raison est tout.
        script::Script s = parse("OUTPUT_FORMAT { STACK = [0x3FFF] }\n");
        ok("une plage a une seule borne est refusee", !s.ok);
        ok("et par le meme refus", says(s, "SP moves"));
    }
    {
        script::Script s = parse("OUTPUT_FORMAT { STACK = [0x4000..0x3F00] }\n");
        ok("une plage a l'envers est refusee", !s.ok);
        ok("et le refus le dit ainsi", says(s, "ends before it starts"));
    }

    // --- Une machine, et une seule ------------------------------------------
    {
        script::Script s = parse("TARGET cpc6128\nTARGET zx128\n");
        ok("deux TARGET sont refuses", !s.ok);
        ok("le refus dit pourquoi", says(s, "one machine"));
        ok("et il nomme la SECONDE ligne", !s.errors.empty() && s.errors[0].line == 2);
    }

    // --- Le lexique ---------------------------------------------------------
    {
        // Les quatre notations de nombre que ce projet emploie deja. En refuser
        // une demanderait d'ecrire ses adresses autrement dans un script que
        // dans une source.
        script::Script s = parse("OUTPUT_FORMAT { ENTRY_POINT = &8000 }\n"
                                 "MEMORY_MAP { CONFIG ext_w1<%01> {\n"
                                 "  w1 [OFFSET #100, SIZE 512] { SECTION a } } }\n");
        ok("&hex, #hex, %bin et decimal", s.ok);
        if (!s.ok) printf("    premiere erreur : %s\n", firstError(s).c_str());
        ok("et ils valent ce qu'ils disent",
           s.output.entry == 0x8000 && s.map.size() == 1 && s.map[0].config.arg == 1 &&
           s.map[0].placements.size() == 1 && s.map[0].placements[0].offset == 0x100 &&
           s.map[0].placements[0].size == 512);
    }
    {
        // Les deux styles de commentaire : `//`, que le §6 emploie dans ses
        // exemples, et `;`, avec lequel une source fantams commente.
        script::Script s = parse("// une carte\n"
                                 "TARGET cpc6128   ; le 6128 nu\n"
                                 "MEMORY_MAP { // rien a placer\n"
                                 "}\n");
        ok("les deux styles de commentaire sont ignores", s.ok && s.target == "cpc6128");
    }
    {
        script::Script s = parse("");
        ok("un script vide est licite et ne declare rien",
           s.ok && !s.hasTarget && s.map.empty() && !s.output.hasEntry);
    }
    {
        // `TARGET` nomme LA MACHINE, et le §6 l'employait aussi pour le
        // conteneur. Le bloc fautif a circule : le refus le nomme plutot que de
        // le traiter comme un mot inconnu.
        script::Script s = parse("OUTPUT_FORMAT { TARGET = \"SNA_V2\" }\n");
        ok("TARGET dans OUTPUT_FORMAT est refuse", !s.ok);
        ok("et le refus nomme CONTAINER",
           says(s, "CONTAINER") && says(s, "names the machine"));
    }
    {
        script::Script s = parse("OUTPUT_FORMAT { CONTAINER = \"SNA_V2\n");
        ok("une chaine non terminee est refusee", !s.ok);
        ok("et le refus nomme sa ligne", !s.errors.empty() && s.errors[0].line == 1);
    }
    {
        script::Script s = parse("MEMORY_MAP { CONFIG linear { w1 { SECTION main }\n");
        ok("une accolade manquante est refusee", !s.ok);
        ok("et le refus nomme la fin de fichier plutot que rien",
           says(s, "end of file"));
    }
    {
        script::Script s = parse("TARGET cpc6128 $\n");
        ok("un caractere inattendu est refuse", !s.ok);
        ok("et il est cite", says(s, "'$'"));
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
