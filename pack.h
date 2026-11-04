// pack.h - Le builder : d'une image liée aux artefacts livrés (ADR 0007)
//
// image → découpage → morceaux → encapsulation → conteneur → artefacts
//
// Un appelant ne voit qu'une image et une Destination qui entrent, des
// artefacts et des diagnostics qui sortent. La profondeur dépend du format :
// seul le DSK passe par le découpage et l'encapsulation (en-tête AMSDOS) ; le
// CPR et le CRO prennent la banque de ROM entière, le SNA l'image entière, le
// binaire brut l'intervalle contigu.
//
// `package` ne lit ni n'écrit aucun fichier : ce qu'il faut savoir du disque —
// une disquette, une cartouche ou une base déjà là — entre comme donnée de la
// Destination, et c'est l'appelant qui écrit les artefacts.
#pragma once

#include "asm.h"
#include "cro.h"
#include "link.h"
#include "profile.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pack {

// Un fichier livré : un chemin relatif, et ses octets.
struct Artifact {
    std::string path;
    std::vector<uint8_t> bytes;
};

// Une disquette AMSDOS, au format DATA.
struct Dsk {
    // Le source principal (le premier fichier d'entrée), tel que tapé : un
    // morceau SEUL prend son nom (CONTEXT.md, Morceau). Chemin et extension
    // sont retirés ici, pas par l'appelant.
    std::string program;
    // La disquette sur laquelle les fichiers sont ajoutés ou remplacés par nom ;
    // vide pour une disquette neuve. Standard ou EXTENDED, format DATA.
    std::vector<uint8_t> existing;
};

// Une cartouche CPC Plus. Elle prend la banque `crom` ENTIÈRE que l'image
// remplit, pas des morceaux : une invocation, une banque (cpr.h).
struct Cpr {
    // La banque physique (0..31) que CE lien produit, telle que donnée. L'image
    // ne la porte pas : c'est le script `-T` qui l'a choisie.
    std::optional<int> bank;
    // La cartouche à laquelle la banque est ajoutée ou dans laquelle elle est
    // remplacée ; vide pour une cartouche neuve.
    std::vector<uint8_t> existing;
    const profile::Profile *profile = nullptr;
};

// Un conteneur de ROMs. Comme le CPR, il prend la banque de ROM entière que
// l'image remplit — une seule (cro.h, ADR 0033).
struct Cro {
    // Le numéro de la ROM, tel que donné : obligatoire pour `rom_hi<n>` et
    // `crom<n>`, refusé pour `rom_lo`.
    std::optional<int> rom;
    cro::GroupOptions group;
    // Le libellé d'un groupe créé, quand `group` n'en déclare pas.
    std::string defaultLabel;
    // Le conteneur auquel la ROM est ajoutée ou dans lequel elle est
    // remplacée ; vide pour un conteneur neuf.
    std::vector<uint8_t> existing;
    const profile::Profile *profile = nullptr;
};

// Le binaire brut : l'intervalle contigu d'adresses logiques que le linker
// produit, sans en-tête. Il n'a pas de place pour dire « et ces octets-là
// sont en banque 5 ».
struct Raw {};

// Un snapshot. Ce n'est pas un conteneur : il prend l'image ENTIÈRE, posée
// sur une base s'il y en a une (sna.h, ADR 0012).
struct Sna {
    // Les octets du snapshot de référence, tels que lus ; vides sans base.
    std::vector<uint8_t> base;
    // Le nom sous lequel la base est désignée dans les diagnostics et le bilan.
    std::string baseName;
};

// Le format de sortie demandé, et ce qu'il faut savoir de l'existant.
using Destination = std::variant<Raw, Sna, Cpr, Cro, Dsk>;

struct Result {
    bool ok = true;
    // Pour un DSK : un seul artefact, l'image disque entière (`path` vide —
    // c'est l'appelant qui sait où il l'écrit). Aucun en cas d'erreur.
    std::vector<Artifact> artifacts;
    std::vector<asmb::Diagnostic> errors;
    std::vector<asmb::Diagnostic> warnings;
    // Le bilan d'un export réussi, une ligne par entrée, sans le chemin de
    // sortie (que seul l'appelant connaît).
    std::vector<std::string> notes;
};

Result package(const link::Image &img, const Destination &dest);

} // namespace pack
