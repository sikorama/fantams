// pack.h - Le builder : d'une image liée aux artefacts livrés (ADR 0007)
//
// image → découpage → morceaux → encapsulation → conteneur → artefacts
//
// Le découpage et l'encapsulation (en-tête AMSDOS) sont internes : un appelant
// ne voit qu'une image qui entre et des artefacts nommés qui sortent. Le DSK est
// le seul conteneur qui passe par ici ; `sna`, `cpr` et `cro` ont leur
// propre chemin.
//
// `package` ne lit ni n'écrit aucun fichier : ce qu'il faut savoir du disque —
// une disquette déjà là — entre comme donnée du conteneur, et c'est l'appelant
// qui écrit les artefacts.
#pragma once

#include "asm.h"
#include "link.h"

#include <cstdint>
#include <string>
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

struct Result {
    bool ok = true;
    // Pour un DSK : un seul artefact, l'image disque entière (`path` vide —
    // c'est l'appelant qui sait où il l'écrit). Aucun en cas d'erreur.
    std::vector<Artifact> artifacts;
    std::vector<asmb::Diagnostic> errors;
    std::vector<asmb::Diagnostic> warnings;
};

Result package(const link::Image &img, const Dsk &dsk);

} // namespace pack
