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

// --- Le chemin de la CLI : une banque a la fois --------------------------
// `cpr.h` (plus haut) l'impose déjà : chaque banque physique est liée
// SÉPARÉMENT. `build()` suppose que l'appelant a déjà TOUTES les images en
// main à la fois (les tests, un futur harnais). La CLI, elle, assemble et
// lie une SEULE fois par invocation (`fantams` §asm_main.cpp) : il lui faut
// un chemin qui ajoute une banque à un `.cpr` déjà existant sur disque,
// sans jamais tenir plus d'une `link::Image` en mémoire.

// Les 16 Ko utiles d'UNE image déjà liée (les octets réellement écrits sur
// la banque `crom<n>`, le reste à zéro) — sans dire pour quel `n` : le
// profil ne le sait pas plus que `build()` ne le savait déjà, c'est
// l'appelant (le script `-T` qu'il a choisi) qui le nomme.
//
// `false` si le profil ne déclare pas l'axe `cart_rom` (`error` nomme l'axe
// manquant) OU si rien n'a été écrit dessus (`error` reste vide : ce n'est
// pas un refus, juste rien à ajouter).
bool extractOne(const link::Image &img, const profile::Profile &profile,
                 std::vector<uint8_t> &bankBytes, std::string &error);

// Insère ou REMPLACE le chunk `physicalId` dans un conteneur déjà
// sérialisé — `container` vide en construit un nouveau. Reconstruit la
// totalité (un `.cpr` n'a pas de mise à jour partielle), donc son coût est
// celui du fichier entier, pas celui d'un octet ajouté.
//
// Échoue si `container` n'est pas vide et n'est pas un `.cpr` bien formé
// (`error` le dit) : mieux vaut un refus qu'un conteneur qui grossit sur un
// malentendu.
std::vector<uint8_t> merge(const std::vector<uint8_t> &container, int physicalId,
                            const std::vector<uint8_t> &bankBytes,
                            const profile::Profile &profile, std::string &error);

} // namespace cpr
