---
status: accepted
---

# `opcode()` : un octet d'encodage fixe, jamais une valeur de programme

`opcode("instruction"[, index[, len]])` est un nouvel opérateur arithmétique,
au même rang que `hi()`, `lo()` et `bankof()` : il rend un entier, utilisable
partout où une expression l'est. Il extrait un ou plusieurs octets de
l'encodage d'une instruction Z80 passée en chaîne — `ld a,opcode("ld (bc),a",0)`
vaut `ld a,2`. Il ne résout ni symbole ni PC, et ne lit que les octets dont la
valeur ne dépend d'aucun opérande variable : tout le reste est une erreur de
compilation, jamais un octet deviné.

## Contexte

`sizeof()` (`pp.cpp`) n'est pas le bon modèle : c'est une substitution
textuelle du préprocesseur, qui ne connaît que les `STRUCT` déclarées et ne
passe jamais par `expr::eval`. Les vrais opérateurs arithmétiques —
`hi`/`lo`/`bankof`/`min`/`max` — vivent tous dans une seule méthode,
`expr.cpp::callBuiltin`, appelée quand un identifiant est suivi de `(`.
`opcode()` s'y greffe de la même façon.

Le seul précédent de chaîne en position d'expression est le littéral
`'…'`/`"…"` (ADR 0010), volontairement limité à un octet : au-delà, la
question « dans quel ordre, avec quelle valeur pour le reste ? » n'a pas de
réponse qui ne soit pas devinée. `opcode()` est le premier cas où une chaîne
plus longue a un sens en expression — mais ce n'est pas la chaîne elle-même
qui vaut un nombre, c'est un octet de son *encodage*, ce qui déplace
entièrement le problème : plutôt que d'assouplir la limite d'un octet, il
s'agit d'aller chercher l'octet ailleurs, dans l'assembleur.

## Ce qui est décidé

**Deux catégories d'opérandes, jamais interchangeables.** Un opérande qui
produit un octet séparé après l'opcode — l'immédiat de `ld a,n`, l'adresse de
`ld (nn),a`, le déplacement de `(ix+d)`, celui d'un `jr`/`djnz` — s'écrit avec
un **placeholder** : `n`, `nn`, `d`, `e`, `imm`, `imm8` ou `imm16`,
interchangeables entre eux, insensibles à la casse. Un opérande qui change au
contraire l'octet d'opcode lui-même — le numéro de bit de `BIT`/`SET`/`RES`,
le vecteur de `RST`, le mode de `IM` — s'écrit avec une vraie valeur
numérique : `opcode("bit 3,(hl)")`, jamais `opcode("bit n,(hl)")`. Écrire un
placeholder là où une valeur réelle est exigée, ou l'inverse, est une erreur
de compilation. Les sept placeholders ne sont pas distingués aujourd'hui —
n'importe lequel convient dans n'importe quelle position variable — parce que
rien ne le demande encore : le jour où un opérande de forme diffère du reste
selon la taille annoncée, la représentation générique existe déjà et n'a
qu'à devenir stricte.

**Aucun symbole, aucun PC.** La chaîne ne résout aucun nom : ni label, ni
constante, ni variable. Et il n'existe pas de PC d'émission pour une
instruction évaluée hors du flux d'assemblage — le déplacement relatif d'un
`jr`/`djnz` n'a donc pas de valeur, quand bien même son octet d'opcode fixe
(le premier) en a une et reste extractible.

**`index`/`len`, dans l'ordre mémoire par défaut.** `index` (par défaut 0)
désigne le premier octet demandé ; `len` (par défaut 1) le nombre d'octets.
Pour `len` positif, les octets sont composés dans l'ordre où ils apparaissent
dans l'encodage — `byte(index)` est le poids fort. Un `len` négatif prend les
mêmes `|len|` octets mais inverse l'affectation : `byte(index)` devient le
poids faible, ce qui compose une valeur little-endian sans changer quels
octets sont lus. C'est un choix réversible à l'usage, pas une conviction : si
la lecture little-endian s'avère la plus fréquente, le signe par défaut
pourra changer sans toucher au reste du contrat.

**Toute lecture qui ne serait pas fixe est un refus, jamais un zéro.** Un
octet dérivé d'un placeholder, d'un déplacement relatif ou de toute
résolution différée n'a pas de valeur ; le demander via `index`/`len` est une
erreur explicite, nommant l'octet en cause. Un `index`/`len` qui dépasse la
longueur réelle de l'encodage l'est tout autant. C'est la même ligne que
l'ADR 0010 pour le littéral de chaîne : deviner un octet — zéro, ou une
convention d'endianness que rien dans le source n'énonce — serait du faux
émis sans le dire.

## Architecture

L'extraction ne vit pas dans `expr.cpp`, qui resterait sinon dépendant de
`parser` et `z80` — un lien nouveau et disproportionné pour un seul
opérateur. Elle vit dans un module dédié, `opcode.h`/`opcode.cpp`, qui
dépend des deux et reste testable seul, comme `z80_test.cpp`. Il fait passer
la chaîne par `parser::parseLine` puis `z80::encode`, avec un `IAsmContext` de
sandbox qui refuse toute résolution de symbole et marque, octet par octet,
ce qui provient d'un placeholder ou d'une relocalisation non résolue.

Le câblage dans `expr::eval` se fait par une **surcharge**,
`eval(text, resolver, opcodeHook = nullptr)`, plutôt qu'en changeant la
signature de `Resolver` ou celle des appels existants. Seul l'assembleur
câble le hook réel ; dans un contexte où il ne l'est pas — la résolution de
variables pures du préprocesseur, par exemple — `opcode()` échoue avec un
message qui le dit, plutôt que de se comporter différemment de contexte en
contexte.

## Conséquences

`opcode` rejoint les mots réservés (ADR 0015), comme `sizeof` et `bankof`.
Les tests suivent le modèle d'`expr_test.cpp` (cas absolu, cas d'erreur au
message explicite) plutôt que celui de `pp_test.cpp`, qui teste un mécanisme
différent — `opcode()` est un opérateur d'expression, pas une substitution
textuelle.
