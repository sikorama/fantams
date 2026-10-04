// timing.h - Durée d'une instruction Z80 : en T-states, et en NOPs du CPC
// (ADR 0035)
//
// Autonome comme z80.cpp, dont elle ne dépend que par `z80.h` : elle ne connaît
// ni adresse, ni symbole, ni machine. Une instruction y est une forme — un
// mnémonique et la nature de ses opérandes — jamais une valeur.
//
// Deux colonnes, et la seconde n'est PAS dérivée de la première :
//
//   - le T-state est la grandeur du Z80 (documentation Zilog) ;
//   - le NOP est celle du Gate Array du CPC, qui étire chaque phase d'une
//     instruction (chaque cycle machine) au multiple de 4 T-states supérieur.
//     La durée en NOPs est donc la SOMME des phases arrondies, pas l'arrondi du
//     total : `push` dure 11 T-states mais 4 NOPs (5-3-3 → 2+1+1), et
//     `out (c),r` fait exception à la règle. Aucune fonction des T-states ne
//     rend ces nombres : c'est une table, mesurée sur machine.
//
// La table des NOPs est celle d'une MACHINE. Elle vit ici parce que le cœur ne
// sait pas la dériver, mais c'est le profil qui dit si elle s'applique
// (`NOP_TABLE`, ADR 0035) : sous un profil qui ne la déclare pas, `nops_between`
// refuse.
#pragma once

#include "z80.h"

namespace timing {

// Le coût d'une instruction, ou la raison pour laquelle elle n'en a pas.
//
// Une instruction dont la durée dépend du chemin (`jr nz`, `djnz`), d'un
// compteur (`ldir`) ou d'une interruption (`halt`) n'a PAS de coût approché :
// `refusal` dit pourquoi, et les durées ne valent rien. La mesure statique est
// exacte ou refusée (ADR 0035).
struct Cost {
    int tstates = 0;
    int nops = 0;                    // NOPs du Gate Array CPC
    const char *refusal = nullptr;   // non nul : pas de durée fixe
    bool ok() const { return refusal == nullptr; }
};

Cost cost(const z80::Instruction &in);

} // namespace timing
