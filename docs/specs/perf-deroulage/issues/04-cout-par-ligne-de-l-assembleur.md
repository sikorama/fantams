# Coût par ligne de l'assembleur

Status: needs-triage
Type: task

Sur `sintab` plié, 65 536 lignes `db N` coûtent encore ≈ 180 ms à l'assembleur,
contre ≈ 100 ms pour autant de `nop`. `process()` tente `parser::parseLine`
(après `canonicalOrthography`) sur chaque ligne avant de reconnaître les
directives, et chaque ligne passe deux fois (passes 1 et 2).

- Gain estimé : utile seulement sur les longues tables de données, de l'ordre
  de 30 % de l'assembleur — l'assembleur ne pèse plus que ≈ 10 % sur
  `nested_if`.
- Complexité : moyenne. L'ordre « instruction d'abord » porte des règles
  (labels, colonne 1, orthographes tolérées) qu'il faut préserver.
