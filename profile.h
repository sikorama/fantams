// profile.h - Le profil de cible, analysé (fantams)
//
// Le profil décrit **ce que la machine sait faire** : ses fenêtres, ses banques
// et leurs attributs, les états de carte réellement atteignables, et par quelles
// écritures on les atteint. Le script, lui, ne dit que ce que le profil ne peut
// pas savoir — quelle section va où (§6).
//
// **Le profil livré est un TEXTE, embarqué dans le binaire, lu par cet
// analyseur-ci.** `--target <nom>` nomme un de ces textes, `-P mien.prof` le
// remplace, et `--dump-profile` en rend une COPIE. Un porteur, un analyseur, un
// jeu de valeurs : le §7 s'est pris la faute inverse dans la figure et en a tiré
// sa règle — une valeur écrite deux fois est une valeur fausse une fois.
//
// Ce module ANALYSE. Il ne calcule ni recouvrement, ni co-visibilité, ni valeur
// de commutation : ce sont les étapes qui placent et qui vérifient.
//
// **Aucun nom de machine n'apparaît ici**, ni dans aucun autre fichier du
// linker : `profiles.cpp` est le seul, et c'est une donnée, pas du code. Le
// §13.2 en fait un invariant mécanique, et `tests/no_machine_names.sh` le tient.
#pragma once

#include "asm.h"

#include <cstdint>
#include <string>
#include <vector>

namespace profile {

// Une expression de `SELECT`, non évaluée. Ce que ses noms valent — `CODE`, la
// page d'une banque, le paramètre d'un état — n'est connu qu'au moment où une
// section est placée : c'est l'affaire de l'étape qui produit les symboles de
// commutation, pas de l'analyseur.
struct Expr {
    enum Kind { Num, Name, Unary, Binary } kind = Num;
    int64_t num = 0;
    std::string name;              // `CODE`, `PAGE`, ou le paramètre d'un état
    std::string op;                // `|`, `<<`, `~`, `-`, …
    std::vector<Expr> args;
};

// Une plage de l'espace adressable du Z80 où quelque chose peut apparaître.
// C'est elle qui donne son `ORG` à une section (§12.3).
//
// Un profil peut en déclarer PLUSIEURS GRILLES SUPERPOSÉES : sur certaines
// machines, une grille de pages de 16 K couvre tout l'espace pendant qu'une
// grille de 8 K en redécoupe la moitié, les deux actives en même temps. La
// grille n'est pas déclarée : elle se déduit
// de quelles banques apparaissent dans quelles fenêtres.
struct Window {
    std::string name;
    int64_t lo = 0, hi = 0;
    int line = 0;
};

// Une unité de stockage physique susceptible d'apparaître dans une fenêtre.
//
// **Sa taille est déclarée, jamais implicite** : 16 K de RAM ici, 8 K pour une
// mega-ROM ailleurs. Un 16 K câblé dans le linker suffirait à rendre une machine
// indescriptible, et « décrire une machine ne doit pas demander de toucher au
// code du linker » est le seul test qui prouve que ce découpage a servi (§13.1).
//
// Les trois attributs sont portés par la BANQUE, et le §13.1 dit pourquoi aucun
// ne peut l'être par la fenêtre : une ROM est en lecture seule où qu'elle
// apparaisse ; la contention frappe certaines banques, si bien qu'une même
// fenêtre est lente ou non selon ce qui y est paginé ; et une banque qu'un
// contrôleur vidéo ne lit jamais interdit d'y placer une section écran.
struct Bank {
    std::string name;
    bool hasSize = false;
    int64_t size = 0;
    bool readOnly = false;
    bool video = false;
    bool contended = false;
    // Le nombre que porte une banque paramétrique — le `b` d'`ext<b>`, le `n`
    // d'une ROM. C'est lui que `PAGE` vaut dans une expression de `SELECT`.
    bool hasPage = false;
    int64_t page = 0;
    // Son EMPLACEMENT DE RANGEMENT : le numéro sous lequel la machine désigne
    // elle-même ce bloc, et celui que `--sym` imprime déjà dans sa colonne
    // `store` (ADR 0019). Deux banques ne peuvent pas le partager.
    //
    // Il est DÉCLARÉ, pour la même raison que la taille : le déduire de l'ordre
    // des lignes rendrait l'ordre du fichier sémantique, et déplacer deux lignes
    // changerait chaque `.sym` et la disposition de chaque snapshot sans un mot.
    bool hasStore = false;
    int64_t store = 0;
    int line = 0;
};

// Ce qu'un état donne à une fenêtre.
struct Slot {
    std::string window;
    std::string bank;
    // La banque peut être nommée paramétriquement — `ext<b>` — et son paramètre
    // est alors celui de l'état, résolu au placement.
    bool hasParam = false;
    std::string param;       // le nom du paramètre, `b`
    int64_t value = 0;       // ou sa valeur, si elle était littérale
    bool literal = false;
};

// Un ÉTAT DE CARTE nommé : pour les fenêtres qu'il concerne, quelle banque y
// apparaît. C'est la notion sans laquelle on écrirait `SELECT <fenêtre>,
// <banque>`, qui suppose chaque fenêtre choisie indépendamment — hypothèse que
// le matériel refuse (§13.1).
struct State {
    std::string name;
    bool hasParam = false;
    std::string param;             // `b` dans `ext_w1<b>`
    bool hasCode = false;
    Expr code;                     // `[CODE %1bb]`
    std::vector<Slot> slots;
    int line = 0;
};

// Une écriture de commutation. `MASK` dit **quels bits du port appartiennent à
// cet axe** — ce que le §12.3 exige sous le nom `__mask_<axe>`, pour qu'un
// source puisse écrire `(état & ~masque) | valeur` sans toucher aux axes
// voisins. Sans lui, sortir la seule valeur de son axe sur un port qui en porte
// quatre écraserait les trois autres, en silence.
struct Write {
    enum Kind { Out, Poke } kind = Out;
    Expr port;
    bool hasMask = false;
    Expr mask;
    Expr value;
    int line = 0;
};

// Comment on atteint un état : les nombres. Les CONTRAINTES vérifiables — pile
// hors d'une plage, fenêtre d'exécution interdite, verrou, séquence imposée —
// appartiennent à l'étage qui les vérifie, et l'analyseur les refuse en le
// disant.
struct Select {
    std::vector<std::string> axes;    // `SELECT a..b = …` en concerne plusieurs
    std::vector<Write> writes;
    int line = 0;
};

// Un AXE de configuration. L'état de la machine est le produit des axes.
//
// `OVER` déclare qu'un axe recouvre un autre — sur certaines machines une ROM
// cache la RAM **en lecture** seulement, l'écriture continuant d'atteindre la
// banque. C'est le profil qui déclare cette priorité, parce que la lire à
// l'envers ferait déclarer conforme un octet écrit dans le vide.
struct Axis {
    std::string name;
    std::string over;              // vide si l'axe ne recouvre rien
    std::vector<State> states;
    int line = 0;
};

// `CONST <nom> = <expr>` : une valeur littérale du profil, indépendante de tout
// placement — un port de commutation qui ne varie pas par axe (`GA_PORT`), par
// exemple. Calculée à l'analyse, pas au calcul de commutation : son expression
// ne peut donc contenir ni `CODE`, ni `PAGE`, ni le paramètre d'un état, ni la
// référence à un autre `CONST` (ADR 0032, décision 1 ; voir `docs/spec-etage-e.md`
// §E1 pour ce que ça exclut et pourquoi).
struct Const {
    std::string name;
    int64_t value = 0;
    int line = 0;
};

struct Profile {
    bool ok = true;
    bool hasTarget = false;
    std::string target;
    std::vector<Window> windows;
    std::vector<Bank> banks;
    std::vector<Axis> axes;
    std::vector<Select> selects;
    std::vector<Const> consts;
    std::vector<asmb::Diagnostic> errors;
};

// Analyse un profil. `file` est le nom que les diagnostics citent ; il n'est
// jamais ouvert.
Profile parse(const std::string &text, const std::string &file);

// --- Les profils LIVRÉS, embarqués dans le binaire (§9) ---------------------
// Le texte d'un profil livré, ou une chaîne vide si ce nom n'en désigne aucun.
// C'est une COPIE de ce que l'analyseur lira : `--dump-profile` le rend tel
// quel, et il n'existe aucun écrivain qui pourrait en diverger.
std::string builtin(const std::string &name);

// Les noms livrés, pour qu'un `--target` inconnu puisse les lister.
std::vector<std::string> builtinNames();

} // namespace profile
