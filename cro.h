// cro.h - Export conteneur de ROMs (.cro, Logon System)
//
// Un CRO est un CONTENEUR (CONTEXT.md, section Export) : un fichier RIFF de
// forme `CRO ` qui range des ROMs dans des GROUPES DE ROMS. Chaque ROM y porte
// un type, un slot et un numéro physique — que fantams ne demande jamais : ils
// se déduisent de la banque que l'image remplit et du numéro passé par la CLI
// (ADR 0033).
//
// Comme pour le `.cpr`, une `link::Image` ne porte qu'une ROM : `rom_hi<n>` et
// `crom<n>` ont un STORE fixe pour toute leur famille, et `n` n'est conservé
// nulle part dans l'image. La CLI lie donc une ROM par invocation, l'extrait
// (`extractOne`), puis la fusionne dans le `.cro` déjà sur disque (`merge`).
#pragma once

#include "link.h"
#include "profile.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cro {

// Les valeurs du chunk RTYP.
enum : uint32_t { kLow = 0, kHigh = 1, kBankable = 2, kMf2 = 3 };

// Les deux familles qu'un groupe ne mêle jamais (ADR 0033) : les ROMs de
// l'ancienne gamme (`rom_lo`, `rom_hi<n>`) et les ROMs de cartouche (`crom<n>`).
enum class Family { Classic, Cartridge };

// Une ROM prête à ranger dans un groupe.
struct Rom {
    Family family = Family::Classic;
    std::string id;           // RID, dérivé de l'emplacement
    uint32_t type = kHigh;    // RTYP
    uint32_t slot = 0;        // RLOG : le « numéro logique » du format
    uint32_t physical = 0;    // RPHY
    std::vector<uint8_t> data;  // RDT : la banque entière, zéro où rien n'est écrit
};

// La ROM qu'UNE image déjà liée remplit. `romNumber` est le `n` de
// `--cro-rom`, -1 s'il est absent : obligatoire pour `rom_hi<n>` et `crom<n>`,
// refusé pour `rom_lo`, qui n'en a pas.
//
// `false` avec `error` rempli si l'image ne remplit aucune banque de ROM
// connue, en remplit plusieurs, ou si `romNumber` ne convient pas à celle
// qu'elle remplit.
bool extractOne(const link::Image &img, const profile::Profile &profile, int romNumber,
                Rom &out, std::string &error);

// Ce que la CLI dit du groupe. Un champ non déclaré laisse la valeur d'un
// groupe existant intacte, et prend son défaut à la création.
struct GroupOptions {
    uint32_t group = 0;                      // GNUM
    bool hasLabel = false;
    std::string label;                       // GLBL ; défaut : `defaultLabel`
    bool hasMask = false;
    uint32_t mask = 0xFFFFFFFF;              // GMSK ; défaut : aucun masque
};

// Insère ou REMPLACE `rom` dans le groupe `options.group` d'un conteneur déjà
// sérialisé — `container` vide en construit un nouveau. Dans un groupe, une
// ROM est désignée par son numéro physique. Tout chunk que fantams ne connaît
// pas est conservé, à sa place.
//
// Renvoie un vecteur vide et remplit `error` si `container` n'est pas un CRO
// bien formé, ou si `rom` mêlerait deux familles dans un groupe.
std::vector<uint8_t> merge(const std::vector<uint8_t> &container, const Rom &rom,
                           const GroupOptions &options, const std::string &defaultLabel,
                           std::string &error);

} // namespace cro
