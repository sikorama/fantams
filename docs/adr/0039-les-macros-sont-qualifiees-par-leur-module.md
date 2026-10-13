---
status: accepted
---

# Les macros sont qualifiées par leur module, leur corps se lit côté appelant, `@` ancre à la racine

Une macro définie dans `MODULE m` s'appelle `m.a()` et se nomme `m.a`. Les noms
écrits dans le corps d'une macro se résolvent **là où elle est appelée**, comme si
le corps était écrit à cet endroit. `@` devant un nom, à l'usage comme à la
définition, l'ancre à la **racine**, sans préfixe de module ni recherche de repli.
Cette décision remplace la clause « les macros restent globales » de l'ADR 0037.

## Contexte

Trois retours d'utilisation, sur un même exemple (une constante `CONST` globale, une
autre dans `MODULE main`, une macro définie hors module qui lit `CONST`, appelée dans
`main`) :

- **Les macros n'avaient pas de module.** Deux bibliothèques qui définissent chacune
  une macro `a` entraient en collision, alors que leurs labels et leurs constantes
  étaient déjà séparés.
- **Le corps de la macro ignorait le module de l'appelant.** `LD DE,CONST` devenait
  `main.CONST` mais le `LD HL,CONST` de la macro restait `CONST` : deux lignes écrites
  au même endroit, lues dans deux modules.
- **`@` à l'usage ne servait à rien.** Dans un module, `@K` passait tel quel dans
  la sortie, et il n'y avait aucun moyen d'écrire « la constante globale » depuis un
  module ou depuis une macro.

## Décision

**Une macro prend le préfixe du module où elle est définie**, composé comme pour les
labels (`a.b.macro`). Dans un module, `a()` trouve `m.a` sans préfixe, avec la règle
de recherche des labels : le nom du module le plus interne d'abord, puis les modules
englobants, puis le global. Depuis l'extérieur, il faut écrire `m.a()`. Une macro
appelée sans qualification alors qu'elle n'existe que dans un module est refusée par
une erreur nommée qui propose le nom qualifié : `unknown macro 'a' — did you mean
'm.a'?`. Il n'y a pas de repli silencieux, comme pour les autres noms (ADR 0037).

**Le corps d'une macro se lit côté appelant.** Un nom du corps — label, constante,
structure, autre macro — est cherché depuis le module de l'appelant, avec le même
masquage : un nom que le module définit l'emporte sur le global. La macro est du
texte déroulé dans le module appelant, ce qui est aussi le comportement attendu par
les sources écrites pour d'autres assembleurs. Pour viser le global depuis un corps,
on écrit `@CONST`.

**`@` ancre à la racine, à l'usage comme à la définition.**
- À l'usage, `@toto.CONST` est le chemin complet depuis la racine, et `@CONST` la
  constante globale. Ceci vaut pour les labels, les constantes, les structures et les
  appels de macro (`@m.a()`).
- À la définition, `@entry:` définit `entry` à la racine, même dans un module :
  dans une macro il garde le sens de l'ADR 0038 (global, une seule expansion), et
  la macro appelée depuis `main` définit `entry`, pas `main.entry`.
- `@x:` dans un module, hors macro, définit `x` à la racine. Au niveau supérieur,
  `@x:` reste l'erreur nommée de l'ADR 0038 : il n'y a rien à ancrer.

## Conséquences

- **Rupture assumée** (ADR 0025). Une macro définie dans un module et appelée sans
  qualification depuis l'extérieur est refusée, avec le message ci-dessus. Le
  correctif est mécanique.
- **Une macro n'a plus de lecture lexicale.** Sa définition ne fixe pas la
  signification des noms de son corps : c'est l'appelant qui la fixe. Une macro de
  bibliothèque qui veut un nom précis l'écrit avec `@`, ou le reçoit en argument.
- **Les exemples** (`examples/struct_module.asm`, `examples/demo.asm`) et
  `docs/syntax.md` suivent.

## Écartée

La lecture côté définition (`CONST` désigne la constante vue là où la macro est
écrite). Elle est plus hygiénique, mais elle oblige à retenir deux environnements par
appel et casse les sources qui comptent sur le module de l'appelant.
