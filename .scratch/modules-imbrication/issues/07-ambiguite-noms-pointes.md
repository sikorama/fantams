Status: resolved
Blocked by: 06

# 07: Ambiguïté des noms pointés : erreur à la définition

Règle (proposée, à acter dans l'ADR 08) : un nom pointé est ambigu si un de ses
préfixes est aussi un module, une structure ou un label ayant des locaux, et si
le même chemin complet est défini d'une autre façon. L'erreur est levée à la
**définition**, et nomme les deux côtés.

Cas réels :
- module `gfx` + structure `gfx` (déjà possible aujourd'hui) ;
- label `gfx.plot` suivi de `.x` produit `gfx.plot.x`, et un module `gfx.plot`
  contenant `x` produirait le même nom.

**Critère** : chaque cas ci-dessus est une erreur au premier des deux
définitions, avec un message qui cite les deux. Tests dans `tests/pp_test.cpp`
et, si besoin, `tests/asm_test.cpp`.

## Réponse

Lecture de la règle : un nom pointé est ambigu quand **le même chemin complet**
est défini de deux façons. C'est déjà une erreur (`duplicate symbol`, au second
des deux) ; il manquait de **nommer les deux côtés**.

`asm.cpp` : `defForm_` retient comment chaque nom a été défini (« local label '.x'
of 'gfx.plot' », « a label », « a constant », « a variable ») ; `duplicateMsg`
cite la forme et le `fichier:ligne` de la première définition, et dit « the same
dotted name is defined two ways » quand les formes diffèrent. Une collision de
même forme dit « already defined as … (fichier:ligne) ».

Constaté, cas du ticket :
- label `gfx.plot` + local `.x` + module `gfx.plot` contenant `x` : erreur nommant
  les deux côtés (testé, `tests/asm_test.cpp`) ;
- module `gfx` + structure `gfx` : **reste accepté** tant qu'aucun membre n'a le
  même nom (ex. `gfx.plot` et `gfx.x` ne se heurtent pas) ; avec un membre commun,
  erreur nommant la constante (champ) et le label. Le critère « module + structure
  de même nom = erreur » n'est donc **pas** appliqué : il n'y a pas d'ambiguïté
  sans nom commun. À rouvrir si vous voulez l'interdire d'office.

Limite : l'assembleur voit les noms après préprocesseur, donc il ne distingue pas
« champ de structure » de « constante » ni « label de module » de « label » ; la
provenance module/structure n'est pas dite. `make test` vert (14 suites). Doc :
`docs/syntax.md`, section Modules.
