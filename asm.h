// asm.h - Assembleur 2 passes (fantams)
//
// Entrée : lignes de source DÉJÀ préprocessées (plates : ni macros ni includes).
// Passe 1 : calcule les adresses (ORG) et collecte tous les symboles.
// Passe 2 : encode réellement, avec les références avant résolues.
//
// La taille d'une instruction Z80 dépend du TYPE des opérandes (pas de leur
// valeur), donc la passe 1 obtient des adresses correctes sans connaître encore
// les valeurs des symboles.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace asmb {

struct SourceLine {
    std::string text;
    std::string file;
    int line = 0;
    bool col0 = false;   // true si, dans la source d'origine, la ligne commençait en colonne 1 (sans indentation)
};

struct Diagnostic {
    std::string file;
    int line = 0;
    std::string message;
};

// Ce que l'assembleur sait d'un symbole exportable (`--sym`, ADR 0019).
//
// Il n'y met AUCUNE adresse de rangement : dans une section relocalisable, un
// label n'a pas d'adresse tant que le linker n'a pas placé son fragment. Il dit
// donc OÙ le symbole habite — quel fragment, à quel offset — et c'est le linker
// qui en tire la banque et l'adresse. C'est l'amendement à l'ADR 0019 : la table
// change de maillon SANS changer de format, et son consommateur — désassembleur
// ou émulateur — ne voit pas la différence. C'est même la raison de le faire
// ainsi.
//
// Une CONSTANTE n'habite nulle part : `frag` vaut -1, ce que la table rend par
// un tiret. Les VARIABLES ('=') n'entrent pas dans la table — leur valeur change
// en cours de route, et un desassembleur n'en ferait rien.
struct Symbol {
    std::string name;       // tel que l'assembleur le connait : QUALIFIE et MANGLE
    bool isConst = false;   // EQU ; sinon label
    // Un symbole est LOCAL à son objet par défaut ; `PUBLIC` l'exporte (§4.4).
    // Le défaut est local pour que deux fichiers puissent employer le même nom
    // de label interne sans se heurter.
    bool isPublic = false;
    int64_t value = 0;      // adresse logique, ou valeur de la constante
    int frag = -1;          // le fragment qui le porte, -1 pour une constante
    int offset = 0;         // son offset dans ce fragment
    // La section qui PORTE le symbole (§4.1). Vide hors de toute section — le
    // cas d'une source qui n'en declare aucune, et celui d'une constante, qui
    // n'habite nulle part.
    std::string section;
    std::string file;       // fichier D'ORIGINE, avant preprocesseur
    int line = 0;           // ligne dans ce fichier
};

// La LIGNE qui a écrit un octet. `Fragment::prov` y renvoie, et c'est ce qui
// permet au linker de nommer les deux lignes en conflit dans un recouvrement.
struct Site {
    std::string file;
    int line = 0;
};

// Un FRAGMENT : un bloc d'octets CONTIGU, appartenant à une section, et
// connaissant son adresse de rangement si un `org` la lui a donnée (D1). C'est
// l'unité que le linker place.
//
// `prov` est PARALLÈLE à `bytes` et porte les deux faits à la fois : zéro = cet
// octet n'a pas été écrit — un trou réservé par `ds` — et non nul = il l'a été,
// par la ligne que `Object::sites[prov - 1]` donne. La coverage de l'ADR 0012
// est exactement « prov non nul » : elle voyage donc AVEC ses octets, dans le
// même objet, et non dans un tableau parallèle que l'appelant doit penser à
// passer.
struct Fragment {
    std::string section;     // vide : des octets hors de toute section
    bool placed = false;     // un `org` lui a donné son adresse
    // La section dont ce fragment attend la base, -1 s'il n'en attend aucune.
    // Quand elle vaut autre chose, `addr` et `logical` sont des OFFSETS dans
    // cette section, et non des adresses.
    int relocSection = -1;
    int addr = 0;            // adresse de RANGEMENT de son octet 0
    int logical = 0;         // adresse LOGIQUE de son octet 0 ; = addr hors bloc déplacé
    int bank = -1;           // banque imposée par un préfixe `org b<n>:`, sinon -1
    std::vector<uint8_t> bytes;
    std::vector<uint16_t> prov;
};

// Ce qu'on sait d'une section (§4.1). Ses fragments se retrouvent par leur nom
// dans `Object::fragments`, où ils gardent leur ORDRE D'ÉCRITURE — l'ordre que
// le placement rejoue, et sans lequel deux recouvrements ne se diagnostiquent
// plus dans le même ordre.
struct Section {
    std::string name;
    int id = -1;             // identité stable, celle que citent les relocalisations
    // Une section SANS `org` est RELOCALISABLE : c'est le linker qui la place,
    // et c'est ce qui donne à « relocalisable » une définition sans nouvelle
    // syntaxe (D1). Une section avec `org` va où son `org` le dit.
    bool relocatable = false;
    std::string kind;        // "RO" / "RW" / "UNINIT"
    bool hasMax = false;
    int64_t max = 0;
    int64_t size = 0;        // octets émis, cumulés sur les réouvertures
    // La ligne de sa PREMIÈRE déclaration — celle qui fixe le type et porte le
    // plafond. Le linker en a besoin : deux unités qui déclarent le même nom
    // sans en dire la même chose se refusent en nommant les deux lignes, et une
    // unité relue depuis un `.fo` n'a pas d'autre moyen de citer la sienne.
    std::string file;
    int line = 0;
};

// Une RELOCALISATION : à cet endroit-ci, il manque la base d'une section, et le
// linker l'y écrira. Les quatre types du §4.6 — et `BankOf` est réservé à C1,
// qui seul connaîtra les banques.
//
//   Abs16  une adresse sur deux octets, petit-boutien
//   Rel8   le déplacement d'un saut relatif, depuis l'octet SUIVANT
//   High8  l'octet de poids fort d'une adresse — ce que `high()` produit
//   Low8   son octet de poids faible — ce que `low()` produit
//   BankOf l'EMPLACEMENT DE RANGEMENT de sa section — ce que `bankof()`
//          produit. Réservé à C1 par l'ADR 0027, parce que seul le linker qui
//          calcule le placement connaît la banque.
//
// L'octet émis à cet endroit ne porte que l'addend ; c'est le linker qui écrit
// la valeur finale, et non qui l'additionne à ce qui s'y trouve. Un objet faux
// se lit alors à l'œil, ce qui vaut plus que tout à l'étage qui introduit la
// relocalisation.
struct Reloc {
    enum Kind { Abs16, Rel8, High8, Low8, BankOf };
    int frag = -1;           // le fragment où elle s'applique
    int offset = 0;          // son offset dans ce fragment
    Kind kind = Abs16;
    // Ce qui manque : la base d'une SECTION de cet objet, ou l'adresse d'un
    // SYMBOLE défini ailleurs. L'un des deux, jamais les deux.
    int section = -1;
    std::string symbol;      // un nom déclaré `EXTERN`, vide sinon
    int64_t addend = 0;      // ce qui s'ajoute à cette base
};

// Un ACCÈS À ADRESSE LITTÉRALE (§4.6, bloc 4) : un `ld (nn),a` dont l'adresse
// est écrite en clair, avec l'endroit d'où il part et ce qu'il vise.
//
// L'assembleur ne l'INTERPRÈTE pas — il ne connaît aucune machine — il
// CONSIGNE. C'est l'étage C2, qui connaîtra le profil, qui y lira une écriture
// dans une plage commutant par accident. Même partage des rôles que la
// relocalisation : l'assembleur note, le linker tranche.
//
// À l'étage B, seules les ÉCRITURES mémoire y figurent : c'est le sens que le
// contrôle d'écriture en `"ro"` produit déjà et teste déjà. Les lectures et les
// ports demandent un parcours d'encodeur que rien ne consomme avant C2.
//
// Un `ld (hl),a` dont HL est calculé n'y figure pas et ne sera **jamais**
// attrapé. C'est la limite du contrôle, et elle est ÉCRITE plutôt qu'à
// découvrir.
struct Access {
    enum Kind { MemWrite };
    int frag = -1;           // le fragment qui porte l'instruction
    int offset = 0;          // son offset dans ce fragment
    Kind kind = MemWrite;
    // L'adresse VISÉE, dite comme une relocalisation : une base de section plus
    // un décalage, ou un nombre tout court quand elle est absolue.
    int section = -1;
    std::string symbol;
    int64_t addend = 0;
};

// Le POINT D'ENTRÉE que `run` a demandé.
//
// C'est un NOM, résolu par le linker : dans une section relocalisable, un label
// n'a pas encore d'adresse, et l'assembleur n'a rien à en dire. `run` accepte
// aussi une adresse littérale — « run #100 » — qui n'a personne à résoudre :
// elle voyage alors telle quelle, `name` vide. Les deux formes existent parce
// que la directive accepte une expression, pas parce qu'il y a deux mécanismes.
struct Entry {
    bool has = false;
    std::string name;        // vide quand `run` portait autre chose qu'un nom
    int64_t value = 0;       // l'adresse, quand `name` est vide
    std::string file;        // la ligne du `run`, pour lui attribuer son diagnostic
    int line = 0;
};

// L'OBJET rendu par l'assembleur : ce qu'une unité de compilation contient, et
// rien de ce qu'il faudrait décider pour la ranger. Aucune image, aucune
// coverage parallèle, aucune banque écrite, aucun binaire, aucune adresse de
// chargement ni d'exécution — ce sont six décisions de PLACEMENT, et elles
// appartiennent au linker (D6).
struct Object {
    bool ok = true;
    // D'OÙ il vient, pour que le linker puisse nommer l'unité fautive quand deux
    // objets se disputent une adresse ou un symbole. Vide n'est pas une erreur :
    // un objet fabriqué à la main dans un test n'a pas de fichier.
    std::string name;
    // Les fragments, dans leur ordre d'écriture. Chacun nomme sa section.
    std::vector<Fragment> fragments;
    std::vector<Section> sections;
    std::vector<Reloc> relocs;
    std::vector<Access> accesses;
    std::vector<Site> sites;     // les lignes citées par `Fragment::prov`
    Entry entry;
    std::map<std::string, int64_t> symbols;
    // La table exportable (ADR 0019) : les memes noms que `symbols`, moins les
    // variables, plus le type et la provenance. Rendue dans l'ordre des noms ;
    // le tri du fichier (banque, rangement, nom) appartient au format, pas ici.
    std::vector<Symbol> symbolTable;
    std::vector<Diagnostic> errors;
    std::vector<Diagnostic> warnings;          // bonnes pratiques (non bloquant) : label sans ':', instruction en colonne 1...
    std::vector<Diagnostic> prints;            // sorties de PRINT (diagnostic de build, ni erreur ni avertissement)
};

Object assemble(const std::vector<SourceLine> &lines);
Object assembleText(const std::string &source, const std::string &file);

} // namespace asmb
