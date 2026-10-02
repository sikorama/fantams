# Réduire les allocations de chaînes dans pp et kw

Status: needs-triage
Type: task

Environ un quart du temps du préprocesseur est hors des fonctions du projet
(malloc, copies). Les helpers `trim`, `upper`, `firstToken`, `restAfterFirst`,
`stripComment` rendent des `std::string` ; chaque ligne en enchaîne plusieurs.
Passer à `std::string_view` là où le résultat n'est que lu.

- Gain estimé : diffus, 10 à 20 %, sans changement de structure.
- Complexité : faible par site, mais beaucoup de sites dans `pp.cpp` et
  `keywords.cpp` — un remaniement mécanique étendu, à risque de durée de vie
  (vue sur un temporaire).
