---
status: accepted
---

# Un seul modèle de durée, trois instruments

La durée d'exécution d'un bloc de code se mesure de trois façons — dans
l'assembleur, au lien, dans un émulateur — et une seule d'entre elles possède le
modèle : l'assembleur. Il porte une table de durées en **T-states** par
instruction ; le profil dit si une table de **NOPs** s'applique (voir l'amendement). Les deux autres instruments consomment ce modèle, ou le confrontent à
une mesure, sans en dupliquer un second.

## Ce qui est décidé

- **Mesure statique** : deux fonctions d'expression, `TSTATES_BETWEEN(a, b)` et
  `NOPS_BETWEEN(a, b)`, de **temps d'assemblage** (comme un label : jamais au
  temps préprocesseur, donc ni `LET`, ni `repeat`, ni `nop n`). Les bornes sont
  `[a, b[`, de sorte que `(a,b) + (b,c) == (a,c)`. Sont **refusés**, nommément :
  `b` avant `a`, deux labels de sections ou d'`org` différents, une donnée
  (`db`, `dw`, `ds`) dans l'intervalle, toute instruction dont la durée dépend
  du chemin (`jr cc`, `djnz`, `call cc`, `ret cc`, `ldir`…) ou qui saute. La
  mesure est exacte ou refusée, jamais approchée.
- **Unités** : le **T-state** est la grandeur du cœur (ADR 0024 : la spec ne
  [Amendé ci-dessous : le NOP n'est pas un arrondi.]   porte aucune valeur matérielle). Le **NOP** — 4 T-states, arrondi
  instruction par instruction — est un attribut du **profil** (ADR 0028) : la constante `NOP_TSTATES`, que l'assembleur reçoit comme `GA_PORT` (le préfixe `__` des `CONST` est réservé aux symboles de commutation, ADR 0032) ;
  `NOPS_BETWEEN` sous un profil sans règle d'arrondi est une erreur qui renvoie
  vers `TSTATES_BETWEEN`. La contention de la RAM n'est pas modélisée, et la
  documentation le dit.
- **Lien** : hors périmètre. Aucun calcul de durée ne dépend aujourd'hui d'une
  adresse finale ; l'instrument reste à décrire quand un cas concret le
  demandera.
- **Mesure dynamique** : décidée dans l'ADR 0036.
- **Vocabulaire** : « cycle » est écarté du glossaire (il désigne aussi bien un
  cycle machine qu'un cycle d'horloge) au profit de **T-state** et **NOP**.
  `CYCLES_BETWEEN` (spec §4.3, jamais figé par un ADR) devient
  `TSTATES_BETWEEN`.

## Pourquoi

Deux modèles de durée — l'un dans l'assembleur, l'autre rétro-déduit des
mesures — divergeraient au premier opcode rare. Le balayage qui confronte la
table à l'émulateur (ADR 0036) est ce qui tient les deux d'accord : si la table
a tort, c'est lui qui le dit, et on corrige la table, jamais la mesure.

`TICKER` reste réservé et refusé (ADR 0025). Il compte dans l'ordre du texte et
ignore le chemin : c'est la raison pour laquelle la mesure statique refuse tout
flot de contrôle plutôt que de le deviner. Le message de refus nomme
`NOPS_BETWEEN` comme remplaçant, comme `BANK` nomme `org b<n>:`.

## Conséquences

- La table des durées s'écrit d'abord depuis la documentation Z80 ; elle n'est
  déclarée juste qu'après le balayage contre l'émulateur.
- Une boucle à compteur n'a pas de mesure statique. C'est le domaine de la
  mesure dynamique, pas une lacune à combler par une approximation.
- `ASSERT NOPS_BETWEEN(a, b) == 19968` est la forme de contrat de durée ;
  le remplissage automatique jusqu'à un budget (`PAD`) est suspendu jusqu'à
  un usage clair.

## Amendement — le NOP n'est pas un arrondi (2026-10-04)

La décision d'origine disait : le NOP vaut 4 T-states, **arrondi instruction par
instruction**, et la règle d'arrondi est un attribut du profil (`NOP_TSTATES`).
C'était faux, et un `out (c),a` compté 3 NOPs au lieu de 4 l'a montré.

Le Gate Array n'arrondit pas le total de l'instruction : il étire **chaque
phase** (chaque cycle machine) au multiple de 4 T-states supérieur. Une
instruction dure donc la somme de ses phases arrondies — `push`, 5-3-3 soit 11
T-states, dure 2+1+1 = 4 NOPs, pas ⌈11/4⌉ = 3 —, et `out (c),r` fait exception à
cette règle même. Plusieurs durées tombent ainsi à un NOP de l'arrondi du total :
`ld hl,(nn)` (5), `ex (sp),hl` (6), `ldi` (5), `push` (4), `in`/`out (c)` (4).
Aucune fonction des T-states ne rend ces nombres.

Ce qui change :

- `timing::cost` rend **deux colonnes**, T-states (Zilog) et NOPs (table mesurée
  de Madram / 64NOPS), au lieu d'une colonne et d'un arrondi. `timing::nops()`
  disparaît.
- Le profil ne porte plus un nombre de T-states par NOP, mais **quelle table de
  NOPs s'applique** : `CONST NOP_TABLE = 1` (1 est celle du Gate Array du CPC).
  `NOP_TSTATES` disparaît. Sous un profil sans `NOP_TABLE`, `nops_between`
  refuse comme avant, et renvoie vers `tstates_between`.

Ce qui ne change pas : le T-state reste la grandeur du cœur, la mesure reste
exacte ou refusée, et le balayage contre l'émulateur (ADR 0036) reste ce qui
tient la table — il aurait attrapé cette erreur, et la mesure de l'émulateur,
pas le calcul, fait autorité.
