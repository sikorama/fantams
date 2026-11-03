# Parseur d'expressions par montée de précédence

Status: needs-triage
Type: task

`expr::eval` descend une fonction par niveau de priorité (logOr → logAnd →
bitOr → … → primary), chacune appelant `skip()` puis essayant ses opérateurs.
Sur `nested_if` ×8 : 655 000 évaluations, 84 millions d'appels à `skip()`,
44 millions à `eat()`. Une montée de précédence (precedence climbing) lit
l'opérateur une fois et consulte une table.

- Gain estimé : la moitié du coût d'évaluation, soit 12 à 15 % du préprocesseur,
  et un peu de l'assembleur (`sintab`).
- Complexité : moyenne. `expr.cpp` est isolé et couvert par `expr_test`. Pièges :
  `**` associatif à droite et plus liant que l'unaire (ADR 0008), les
  opérateurs textuels (`and`, `shl`…) qui exigent une frontière de mot,
  `&&` à ne pas lire comme `&`, `<<` à ne pas lire comme `<`.
