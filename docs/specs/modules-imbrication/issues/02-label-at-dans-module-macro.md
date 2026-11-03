Status: resolved
Blocked by: —

# 02: Un label `@` dans une macro de module est dupliqué à la deuxième expansion

**Symptôme** (reproduit) : `MODULE e` / `macro m(x)` / `@r: nop` / `jp @r` /
`endmacro` / `ENDMODULE` puis deux appels → `duplicate symbol: 'e.@r'`. Hors
module, le même code donne `@r__1`, `@r__2`.

**Cause** : le renommage de module (`pp.cpp`, `onlyAtPrefixed=false`) transforme
`@r` en `e.@r` avant l'expansion. Ensuite `collectLabels(onlyAtPrefixed=true)`
ne reconnaît plus le `@` en tête, donc `renameLocals` n'ajoute aucun suffixe.

**À faire** : les labels `@` ne doivent pas être préfixés par le module, ou
doivent rester reconnaissables après préfixage. Ce correctif ne dépend pas du
schéma de nommage (ticket 09) : il sera réécrit par ce dernier si besoin.

**Critère** : deux expansions dans un module assemblent, et les noms sont uniques.
Test dans `tests/pp_test.cpp`.

## Réponse

Choix : le label `@` reste préfixé par le module (`e.@r`), mais reste
reconnaissable. `pp.cpp` : `isAtName` accepte `@x` et `<module>.@x` ; les trois
tests `label[0] == '@'` de `collectLabels(onlyAtPrefixed)` l'utilisent. Deux
expansions donnent `e.@r__1` et `e.@r__2`. Ne pas préfixer du tout aurait fait
entrer en collision deux modules ayant chacun un `@l` (`duplicate symbol: '@l'`) :
vérifié, ce cas assemble toujours.

Test « MODULE : label @ de macro, préfixé et unique par expansion » dans
`tests/pp_test.cpp` ; `make test` vert (14 suites).
Le ticket 10 réécrira ce mécanisme (scope par expansion).
