# Plier les opérandes d'instruction

Status: needs-triage
Type: task

`pp::fold` ne plie que `db`/`dw`, `equ` et les affectations (ADR 0034). Une
variable lue par une instruction (`ld a, w` dans une boucle) garde donc toutes
ses affectations, et la ligne reste à évaluer en deux passes.

- Gain estimé : nul sur le banc actuel ; réel seulement pour les déroulages de
  code (`ld (hl), k*3` sur 1 000 tours).
- Complexité : moyenne. Plier un opérande sans changer le mode d'adressage :
  `ld a,(2*k)` est un accès mémoire, `(ix+2*k)` un déplacement ; il faut plier
  l'expression à l'intérieur des parenthèses, pas l'opérande entier.
