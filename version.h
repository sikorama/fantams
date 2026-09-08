// version.h — fantams sait dire qui il est.
//
// DEUX DATES, jamais un identifiant de commit : un identifiant de commit depend
// du depot, et le depot pourra etre recree ou son historique aplati. Ce que la
// version doit survivre, c'est precisement cela.
//
//   - la DATE DE VERSION vit dans l'arbre des sources, une ligne plus bas, et le
//     mainteneur l'incremente quand il le decide. Elle dit ce qui a ete VOULU
//     comme livraison.
//   - la DATE DE COMPILATION vient de __DATE__, le macro standard du
//     preprocesseur. Elle dit quand CET artefact-la a ete produit. Elle n'exige
//     ni git, ni reseau, ni option de compilation : elle vaut donc a l'identique
//     dans la distrobox, dans le conteneur de compilation WASM, et sur une
//     archive extraite sans depot.
//
// C'est l'ECART entre les deux qui repond a « cet artefact est-il a jour ? ».
// Un artefact vieux de trois etages le dit alors de lui-meme, ce qui est
// exactement ce qui a manque pendant trois etages.
//
// La chaine est faite pour etre LUE. Aucun ordre entre versions n'est defini,
// rien ne l'analyse, et un changement de sa forme ne doit rien casser.
#pragma once
#include <string>

namespace version {

// La date de version. L'incrementer est ce geste-ci, et rien d'autre.
inline const char *release() { return "2026-09-08"; }

// __DATE__ rendu en AAAA-MM-JJ, pour que les deux dates se lisent dans la meme
// graphie et que l'ecart saute aux yeux. Le macro rend « Mmm JJ AAAA », avec un
// jour cadre sur deux colonnes — espace en tete sous le dixieme du mois.
inline std::string build() {
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const std::string d = __DATE__;               // « Sep  8 2026 »
    if (d.size() < 11) return d;                  // forme inattendue : telle quelle
    const size_t m = std::string(months).find(d.substr(0, 3));
    if (m == std::string::npos) return d;
    char mm[3] = { char('0' + (m / 3 + 1) / 10), char('0' + (m / 3 + 1) % 10), 0 };
    std::string dd = d.substr(4, 2);
    if (dd[0] == ' ') dd[0] = '0';
    return d.substr(7, 4) + "-" + mm + "-" + dd;
}

// Les deux dates, accolees, sur une seule ligne.
inline std::string line() {
    return "fantams " + std::string(release()) + " (compile " + build() + ")";
}

}  // namespace version
