// opcode.h - Extraction d'octets fixes de l'encodage d'une instruction Z80
// (ADR 0031)
//
// Autonome vis-à-vis d'expr/pp : dépend seulement de parser+z80. Ne résout
// aucun symbole, ne connaît aucun PC réel — seuls les octets dont la valeur
// ne dépend d'aucun opérande variable sont extractibles.
#pragma once

#include <cstdint>
#include <string>

namespace opcode {

struct Result {
    bool ok = false;
    int64_t value = 0;
    std::string error;
};

// Extrait `len` octets de l'encodage de `instrText` à partir de `index`
// (0-based, ordre mémoire). `len` négatif prend les mêmes |len| octets mais
// place celui d'`index` en poids faible plutôt qu'en poids fort.
Result extract(const std::string &instrText, int index, int len);

} // namespace opcode
