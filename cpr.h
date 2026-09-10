// cpr.h - Export cartouche CPC Plus (.cpr)
//
// Un CPR est un CONTENEUR (CONTEXT.md, section Export) : il agrège des
// Morceaux SANS Encapsulation, un par banque physique de ROM (0..7, le `n` de
// `crom<n>` du profil `cpcplus`), en un fichier RIFF de forme `AMS!` — un
// chunk nommé `cb00`..`cb1f` par banque.
//
// Une seule `link::Image` ne peut pas porter plusieurs banques `crom<n>` :
// la banque `crom<n>` a un STORE fixe (16) pour toute la famille paramétrique,
// et l'ID physique `n` n'est conservé nulle part dans `Image::blocks`. Chaque
// banque physique doit donc être liée SÉPARÉMENT (un `link::build` par `n`,
// avec un script qui le fixe), et `build` en prend une collection, indexée
// par cet ID physique.
#pragma once

#include "link.h"
#include "profile.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cpr {

// `perBank[n]` est l'image déjà liée pour la banque physique `n` (0..7).
// Une entrée dont l'image n'a rien écrit sur l'axe `cart_rom` ne produit
// aucun chunk : la cartouche ne porte que ce qui a été réellement écrit.
//
// Échoue si `profile` ne déclare pas l'axe `cart_rom` (nomme l'axe manquant
// dans `error`, plutôt que de deviner) : renvoie alors un vecteur vide.
std::vector<uint8_t> build(const std::map<int, link::Image> &perBank,
                            const profile::Profile &profile, std::string &error);

} // namespace cpr
