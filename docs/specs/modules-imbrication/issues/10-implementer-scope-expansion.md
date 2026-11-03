Status: resolved
Blocked by: —

# 10: Implémenter le scope par expansion (remplace le suffixe `@`)

- Remplacer le suffixe `__id` par le préfixe d'expansion décidé au ticket 09.
- Le bug de ticket 02 disparaît par construction : vérifier qu'il reste couvert
  par un test.
- Les symboles de `repeat` et `while` suivent la même règle.
- `renameLocals` (`pp.cpp:1076-1093`) et `collectLabels` sont le point d'entrée.

**Critère** : `tests/pp_test.cpp` et `tests/asm_test.cpp` passent. Le fichier de
symboles (`--sym`) montre les noms par expansion.

## Règles décidées (spec.md, Q6 à Q10)

- Label écrit en dur : local à l'expansion (`m__1.loop`). Label contenant `{…}` :
  global, nom construit. `@x:` : global, `@` retiré du nom, usage unique ; erreur
  nommée hors expansion. `@@export` : erreur nommée. Constantes (`K EQU`) : même règle.
- Masquage comme les modules (`renameModuleScope` est le modèle).
- Le correctif provisoire du ticket 02 (`isAtName`) est supprimé.
- `examples/demo.asm` (`@wloop`) et la doc (`docs/syntax.md`, `CONTEXT.md`) à réécrire.

## Réponse

Implémenté en tranches testées (seams : `pp::preprocess`, CLI `fantams`) ; `make test`
vert (14 suites + `accept_expansion.sh`, ajouté au Makefile et au CMake).

- `pp.cpp` : `collectLabels(body, skipNested, variables)` rend les noms définis
  directement (les régions module/macro/boucle imbriquées nomment les leurs, les
  variables `=` ne sont jamais scopées) ; `renameScoped` généralise l'ancien
  `renameModuleScope` (masquage, lignes `MODULE`/`MACRO` intactes) ; `renameLocals`
  préfixe `<module><expansions>nom__<n>.` ; `runExpansion` compose le préfixe
  (`m__1.repeat__2.x`) ; `@x` : global, `@` retiré, une seule expansion
  (`duplicate symbol: … marks it with '@'` au second appel) ; `@x` hors expansion,
  `@@export` et `__<chiffres>` écrit à la main : erreurs nommées. Un nom suivi de
  `{` ou `(` n'est pas un label (`mon_{name}:`, appel `M(1)`).
- `keywords.*` : `hasExpansionPrefix`, partagé avec `asm.cpp`, où un label d'expansion
  ne devient pas propriétaire des `.locaux` (remplace l'ancien test sur `@`).
- Tests : `tests/pp_test.cpp` (35 cas), `tests/asm_test.cpp`, `tests/accept_expansion.sh`.
- Exemples : `examples/demo.asm`, `examples/struct_module.asm` (`run gfx.main`).
- Doc : `docs/syntax.md` (section Scope, exemples vérifiés à la main), `CONTEXT.md`,
  `README.md`, ADR 0038.

Limites connues : un appel de macro nu (sans parenthèses) dans un module est pris pour
un label au balayage du module (la forme nue est déjà déconseillée) ; la détection
« une seule expansion » est statique (un label `@` sous un `if` non pris compte quand
même).
