// timing.h - Durée d'une instruction Z80, en T-states (ADR 0035)
//
// Autonome comme z80.cpp, dont elle ne dépend que par `z80.h` : elle ne connaît
// ni adresse, ni symbole, ni machine. Une instruction y est une forme — un
// mnémonique et la nature de ses opérandes — jamais une valeur.
//
// Le T-state est la grandeur du Z80 ; le NOP — quatre T-states, arrondi
// instruction par instruction — est celle d'un profil, et ce module n'en sait
// que l'arithmétique (`nops`), pas la valeur : c'est le profil qui la porte.
#pragma once

#include "z80.h"

namespace timing {

// Le coût d'une instruction, ou la raison pour laquelle elle n'en a pas.
//
// Une instruction dont la durée dépend du chemin (`jr nz`, `djnz`), d'un
// compteur (`ldir`) ou d'une interruption (`halt`) n'a PAS de coût approché :
// `refusal` dit pourquoi, et `tstates` ne vaut rien. La mesure statique est
// exacte ou refusée (ADR 0035).
struct Cost {
    int tstates = 0;
    const char *refusal = nullptr;   // non nul : pas de durée fixe
    bool ok() const { return refusal == nullptr; }
};

Cost cost(const z80::Instruction &in);

// Arrondi au NOP : chaque instruction se compte à part, au multiple de
// `tstatesPerNop` supérieur. Arrondir le TOTAL donnerait un autre nombre, et
// c'est le Gate Array qui arrondit, instruction par instruction.
int nops(int tstates, int tstatesPerNop);

} // namespace timing
