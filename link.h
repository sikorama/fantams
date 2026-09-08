// link.h - Le linker : des objets, une image (fantams)
//
// L'assembleur rend un OBJET — ce qu'une unité de compilation contient. Le
// linker en prend N et rend une IMAGE — des octets à des adresses. C'est le
// seul chemin par lequel un octet sort de fantams, de sorte qu'un placement
// faux fait rougir un test le jour où on l'écrit, et non trois étages plus tard.
//
// Il reçoit deux entrées de plus qu'à l'étage B : un SCRIPT — quelle section va
// où — et un PROFIL — ce que la machine sait faire. Un script vide et un profil
// vide sont des valeurs licites, et elles donnent EXACTEMENT le placement
// absolu de l'étage B : c'est ce qui fait du §12.1 une valeur plutôt qu'une
// intention.
//
// Il ne VÉRIFIE encore rien de ce que l'étage C2 vérifiera : ni la continuité et
// ses trois pointeurs, ni la co-visibilité des références, ni les sections
// miroir. C'est ce qui permet de le livrer sans promettre C2.
//
// Ce qui a migré ici, et qui était dispersé dans trois endroits qui ne savaient
// pas qu'ils le faisaient : la dérivation banque↔adresse, la limite des banques
// qu'un dump plat sait porter, le choix 64 K / 128 K, la détection de
// recouvrement, et la reconstitution de l'image plate.
#pragma once

#include "asm.h"
#include "profile.h"
#include "script.h"

#include <cstdint>
#include <string>
#include <vector>

namespace link {

// Un BLOC PLACÉ : les octets d'un fragment, une fois que le linker a décidé de
// leur banque et de leur adresse. Sa coverage voyage AVEC eux, dans le même
// objet — `covered` est parallèle à `bytes`, jamais un paramètre que l'appelant
// doit penser à passer (ADR 0012).
struct Block {
    int bank = 0;
    int addr = 0;                  // adresse de rangement de son octet 0
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> covered;  // 0 = trou réservé, non nul = écrit par la source
};

// Une entrée de la table des symboles exportable (`--sym`, ADR 0019), avec ses
// adresses DÉFINITIVES.
//
// Elle sort du LINKER et non de l'assembleur : dans une section relocalisable,
// un label n'a pas d'adresse tant que sa section n'est pas placée. Le FORMAT ne
// change pas d'une colonne ni d'un en-tête, et son consommateur — désassembleur
// ou émulateur — ne voit pas la différence. C'est même la raison de le faire
// ainsi (amendement à l'ADR 0019).
struct Symbol {
    std::string name;       // QUALIFIE et MANGLE, tel que l'assembleur le connait
    bool isConst = false;   // EQU ; sinon label
    int64_t value = 0;      // adresse logique définitive, ou valeur de la constante
    int bank = -1;          // banque de rangement, -1 pour une constante
    int store = -1;         // adresse de rangement, -1 pour une constante
    std::string section;    // la section qui le porte ; vide hors de toute section
    std::string file;       // fichier D'ORIGINE, avant preprocesseur
    int line = 0;
};

struct Image {
    bool ok = true;
    std::vector<asmb::Diagnostic> errors;
    std::vector<asmb::Diagnostic> warnings;
    // Les blocs, dans l'ordre où ils ont été posés.
    std::vector<Block> blocks;
    // Les banques où la source a RÉELLEMENT écrit, triées.
    std::vector<int> banksWritten;
    // Le binaire brut : un intervalle contigu d'adresses de rangement, dans les
    // 64 K de base. Vide si rien n'y a été écrit.
    std::vector<uint8_t> bin;
    uint16_t loadAddress = 0;      // adresse du premier octet de `bin`
    uint16_t runAddress = 0;       // point d'entrée résolu ; = loadAddress sans `run`
    // La table exportable, avec des adresses définitives. Dans l'ordre des noms ;
    // le tri du fichier (banque, rangement, nom) appartient au format, pas ici.
    std::vector<Symbol> symbolTable;
};

// Lie N objets en une image.
//
// Les deux dernières entrées sont facultatives, et leur absence n'est pas un cas
// particulier : c'est le cas COURANT, celui d'une source à un `org` et un `run`
// (§12.1). Sans script, aucune section n'est placée par calcul, et le placement
// dérivable de l'étage B s'applique tel quel.
Image build(const std::vector<asmb::Object> &objects,
            const script::Script &script = script::Script(),
            const profile::Profile &profile = profile::Profile());

// L'image PLATE des banques 0..7 — l'octet (banque b, offset o) en b*0x4000+o —
// et sa coverage, telles que `sna::build` les attend.
//
// Deux tableaux parallèles ici, et c'est assumé : c'est la signature du backend
// de snapshot qui les demande ainsi. Sa dette est réelle, mais c'est celle du
// builder ; la mêler au linker ferait de l'étage indivisible un étage à deux
// sujets. Le linker, lui, garde la coverage attachée à ses octets.
struct Flat {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> covered;
};
Flat flatten(const Image &img);

// Les banques 0..7 : les 64 K de base plus l'extension du 6128, soit ce qu'un
// dump plat de 128 K sait porter. Au-delà, il faudrait les chunks MEM du v3.
static const int kFlatBanks = 8;

} // namespace link
