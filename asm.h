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

// Une entree de la table des symboles exportable (`--sym`, ADR 0019).
//
// `value` est l'adresse LOGIQUE : ce que le nom vaut dans une expression, et ce
// qu'un desassembleur doit substituer. `bank` et `store` decrivent le RANGEMENT :
// ou l'octet est reellement ecrit. Les deux ne divergent que dans un bloc
// « org <logique>,<rangement> ».
//
// Une CONSTANTE n'habite nulle part : `bank` et `store` valent -1, ce que la
// table rend par un tiret. Les VARIABLES ('=') n'entrent pas dans la table — leur
// valeur change en cours de route, et un desassembleur n'en ferait rien.
struct Symbol {
    std::string name;       // tel que l'assembleur le connait : QUALIFIE et MANGLE
    bool isConst = false;   // EQU ; sinon label
    int64_t value = 0;      // adresse logique, ou valeur de la constante
    int bank = -1;          // banque de rangement, -1 pour une constante
    int store = -1;         // adresse de rangement, -1 pour une constante
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
    std::string kind;        // "RO" / "RW" / "UNINIT"
    bool hasMax = false;
    int64_t max = 0;
    int64_t size = 0;        // octets émis, cumulés sur les réouvertures
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
    // Les fragments, dans leur ordre d'écriture. Chacun nomme sa section.
    std::vector<Fragment> fragments;
    std::vector<Section> sections;
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
