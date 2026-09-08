// script.h - Le script de linkage, analyse (fantams)
//
// Le script dit **quelle section va où**, et rien d'autre. Ce que la machine
// sait faire est dans le profil de cible (§6) ; les deux ne s'écrivent ni au
// même moment, ni par la même personne.
//
// Ce module ANALYSE, il ne résout rien. Un script qui nomme une configuration
// que le profil ne porte pas passe l'analyse sans un mot : le profil n'existe
// pas ici, et c'est l'étape C1.4 — celle qui place — qui refusera. Découper
// ainsi laisse l'analyseur se tester seul, sur du texte, sans un octet.
//
// Fonction PURE : un texte entre, une valeur sort. La lecture du fichier
// appartient au CLI, ce qui rend la syntaxe testable sans toucher au disque —
// même dessin que `fo::read`.
#pragma once

#include "asm.h"

#include <cstdint>
#include <string>
#include <vector>

namespace script {

// Un état de carte NOMMÉ, tel que le script y renvoie : `linear`,
// `ext_w1<1>`, `rom_upper.on`.
//
// L'axe est facultatif parce qu'un nom d'état suffit quand il est unique —
// `linear` — et devient nécessaire quand il se répète d'un axe à l'autre :
// `on` et `off` appartiennent à `rom_lower` comme à `rom_upper`.
struct ConfigRef {
    std::string axis;        // vide si l'état a été nommé seul
    std::string state;
    bool hasArg = false;     // le `<1>` d'un état paramétrique
    int64_t arg = 0;
};

// Un qualificatif de placement : `ROM 15`. Un nom, et une valeur s'il en porte.
struct Qualifier {
    std::string name;
    bool hasValue = false;
    int64_t value = 0;
};

// Ce qu'une fenêtre reçoit, dans une configuration donnée. Les sections y sont
// dans l'ORDRE DU SCRIPT : c'est celui que le placement rejoue, et le seul que
// son auteur peut prévoir.
struct Placement {
    int window = -1;
    // Un découpage de placement À L'INTÉRIEUR de la banque (§13.1) — et non une
    // banque plus petite : les deux moitiés apparaissent ensemble ou pas du tout.
    bool hasRange = false;
    int64_t offset = 0, size = 0;
    std::vector<std::string> sections;
    std::string file;
    int line = 0;
};

struct ConfigBlock {
    ConfigRef config;
    std::vector<Qualifier> qualifiers;
    std::vector<Placement> placements;
    std::string file;
    int line = 0;
};

// Ce que le builder doit savoir, et que le profil ne peut pas savoir : le
// conteneur, le point d'entrée, et deux valeurs que le programme POSE plutôt
// que la machine (§6).
//
// Chaque champ dit s'il a été DÉCLARÉ, et non seulement sa valeur : le script
// ne l'emporte sur le `run` du source que s'il le nomme aussi.
struct Output {
    bool hasFormat = false;
    std::string format;            // "SNA_V2", "CRO", …
    bool hasEntry = false;
    int64_t entry = 0;
    // Une PLAGE, parce que `SP` bouge et qu'une adresse unique ne dit rien de
    // vrai d'un programme qui empile. Elle sert au contrôle de continuité, donc
    // à l'étage C2 ; elle est analysée ici pour n'avoir pas à être rétro-insérée.
    bool hasStack = false;
    int64_t stackLo = 0, stackHi = 0;
    bool hasIntVector = false;
    int64_t intVector = 0;         // DÉCLARÉ, jamais déduit (§6)
    bool hasRomNumber = false;
    int64_t romNumber = 0;
};

struct Script {
    bool ok = true;
    bool hasTarget = false;
    std::string target;                      // `cpc6128`
    std::vector<std::string> extensions;     // `RAM128`, …
    std::vector<ConfigBlock> map;
    Output output;
    std::vector<asmb::Diagnostic> errors;
};

// Analyse un script. `file` est le nom que les diagnostics citent ; il n'est
// jamais ouvert.
//
// Un mot-clé inconnu est une ERREUR, jamais un silence : un script qu'on
// ignorerait à moitié produirait un binaire faux sans un mot.
Script parse(const std::string &text, const std::string &file);

} // namespace script
