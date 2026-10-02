---
status: proposed
---

# L'assembleur consomme la source déroulée pliée, `-E` montre la source fidèle

Le temps d'assemblage assemble la **source déroulée pliée** : chaque expression
résoluble au temps préprocesseur y est remplacée par sa valeur, et des
affectations successives d'une variable seule la dernière subsiste. `-E` continue
d'afficher la source déroulée fidèle, celle qui montre ce que l'auteur a écrit ;
`-E --fold` affiche ce que l'assembleur reçoit réellement.

## Contexte

Une source qui s'appuie sur des boucles imbriquées est lente. Les mesures
(2026-10-02, natif `-O2`) :

| Source | Total | dont préprocesseur |
|---|---|---|
| `repeat 8,j` / `repeat #400,x`, 8 `if` par tour (24 577 lignes déroulées) | 1,05 s | 0,83 s |
| `repeat 65536,ii` + `db 24+20*sin(…)` | 1,17 s | 0,54 s |
| la même, pliée à la main, un `db` par ligne | 0,71 s | 0,30 s |
| les mêmes octets, 16 par ligne | 0,085 s | — |

Le coût dominant est **par ligne**, pas arithmétique : l'assembleur relit le texte
de chaque ligne (label, mot-clé, commentaire) à chaque passe, et réanalyse chaque
expression deux fois. Dans la source à `if`, deux tiers des lignes déroulées sont
des affectations `ii=…` / `w=0` que l'assembleur retraite pour rien. Plier dans
la seule vue `-E` n'y changerait rien. Pour que le gain porte, il faut plier
l'entrée de l'assembleur.

## Ce que coûte la décision

L'invariant « l'assembleur assemble exactement le texte que montre `-E` » cesse
d'être vrai par construction. Il devient une propriété : la source déroulée et la
source déroulée pliée doivent s'assembler aux mêmes octets, et la suite de tests
le vérifie en assemblant les deux formes. Une différence est un bug du pliage, et
`-E --fold` existe pour l'investiguer.

## Règles du pliage

- **Précision** : une valeur entière s'écrit en entier. Une valeur non entière
  s'écrit avec la précision maximale du `double` (`%.17g`), jamais à travers
  `std::to_string`, qui tronque à six décimales : l'ADR 0008 garantit le calcul en
  `double` jusqu'à l'émission, et une variable pliée puis relue ne doit pas
  changer d'octet.
- **Granularité** : on ne plie qu'un opérande **entièrement** résoluble au temps
  préprocesseur. `ld hl, table + 2*ii` reste tel quel. Plier la sous-expression
  obligerait à régénérer un texte d'expression depuis l'arbre, et un écart de
  priorité y serait un bug silencieux.
- **Affectations** : seule la dernière affectation d'une variable subsiste, à sa
  position d'origine, car un label lu au temps d'assemblage après la boucle doit
  voir la dernière valeur. Dès qu'une seule affectation d'une variable n'est pas
  pliable (`w = label+1`), **toutes** ses affectations sont gardées.
- **Positions** : chaque ligne pliée porte la position, dans la source déroulée
  fidèle, de la ligne dont elle provient. Les diagnostics citent cette position :
  une position qui renverrait à un texte invisible sous `-E` ne servirait à rien.
- **Avertissements** : un avertissement né dans une expression pliée est émis
  par le préprocesseur **une fois par ligne d'origine**, avec le nombre
  d'occurrences, et non une fois par tour de boucle (même raison que l'ADR 0016 :
  un diagnostic répété 8 192 fois est un diagnostic qu'on désactive).
  L'avertissement de l'ADR 0008 sur l'opérande non entier n'est pas encore
  implémenté. Il suivra cette règle quand il le sera.

## Options écartées

- **Plier pour l'affichage seulement** : sans effet sur le temps, puisque
  l'assembleur retraiterait la source fidèle.
- **Faire de la source pliée la source déroulée** : on gagne autant, mais on
  perd la formule. Le lecteur ne verrait plus d'où vient `db 24`, alors que la
  source déroulée est un livrable lisible de premier plan.

## Ordre de livraison

Ce pliage vient **après** les corrections locales du préprocesseur (recherche
des mnémoniques en table, `collectLabels` sorti des boucles, corps de boucle
classé une fois) et le passage du build WASM à `-fwasm-exceptions`. On remesure
entre chaque étape. Le statut passe à `accepted` quand le pliage est livré.
