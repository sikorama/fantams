// fo.h - Le fichier objet, écrit et relu (fantams)
//
// Extension **`.fo`**, un format TEXTE à blocs nommés : les sections et leurs
// fragments, les symboles, les relocalisations, et les octets en hexadécimal.
// Diffable, lisible dans un terminal, comparable par chaîne dans un test.
//
// Trois raisons de ne pas le faire compact : un objet faux se lit à l'œil, ce
// qui vaut plus que tout à l'étage qui introduit la relocalisation ; les tests
// s'écrivent contre des chaînes ; et la couture *format* reste hypothétique
// jusqu'à l'étage D, qui apportera le vrai second producteur. Pas `.o`, qui
// laisserait croire à un objet ELF.
//
// La table des symboles de `--sym` en est un SOUS-ENSEMBLE : le bloc `symbol`
// porte les mêmes noms, plus la portée et l'ancrage dans un fragment.
//
// Fonctions PURES : l'une prend un objet et rend une chaîne, l'autre l'inverse.
// L'écriture du fichier appartient au CLI — c'est ce qui rend le format testable
// sans toucher au disque.
#pragma once

#include "asm.h"

#include <string>

namespace fo {

// Le texte d'un objet, terminé par un '\n'. Déterministe : le même objet donne
// toujours le même texte, et c'est ce qui rend l'aller-retour comparable.
std::string write(const asmb::Object &obj);

// Relit un texte d'objet. Rend false et remplit `error` — « line N: ... » — sur
// un fichier malformé, en NOMMANT la ligne fautive : un objet est écrit par une
// machine, mais il se corrige à la main.
bool read(const std::string &text, asmb::Object &out, std::string &error);

} // namespace fo
