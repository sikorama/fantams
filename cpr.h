// cpr.h - Le format cartouche CPC Plus (.cpr)
//
// Un CPR est un CONTENEUR (CONTEXT.md, section Export) : un fichier RIFF de
// forme `AMS!`, un chunk nommé `cb00`..`cb1f` par banque physique de ROM. Il
// prend la banque ENTIÈRE, sans Encapsulation ni découpage en morceaux.
//
// Une `link::Image` ne porte qu'une banque : `crom<n>` a un STORE fixe (16)
// pour toute la famille, et l'ID physique `n` n'est conservé nulle part dans
// l'image. Chaque banque se lie donc séparément, et le builder (pack.h) en
// extrait les octets (`extractOne`) puis les fusionne dans la cartouche déjà
// sur disque (`merge`), sous le numéro que l'appelant lui donne.
#pragma once

#include "link.h"
#include "profile.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cpr {

// Les 16 Ko utiles d'UNE image déjà liée (les octets réellement écrits sur
// la banque `crom<n>`, le reste à zéro) — sans dire pour quel `n` : le
// profil ne le sait pas, c'est l'appelant (le script `-T` qu'il a choisi)
// qui le nomme.
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
