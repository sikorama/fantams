Status: resolved
Blocked by: 01, 05

# 06: Implémenter l'imbrication des modules

- Le handler `MODULE` (`pp.cpp:1554-1584`) cherche aujourd'hui la prochaine
  ligne `MODULE`/`ENDMODULE` : remplacer par une recherche appariée (pile),
  comme `findMatching`. Ajouter `MODULE` à la table des blocs (`pp.cpp:1103`).
- Préfixe composé : `modulePrefix_` est déjà sauvegardé et restauré ; le nouveau
  préfixe est `outer + mname + "."`.
- `collectLabels(skipNestedModule=true)` existe déjà ; vérifier qu'un label d'un
  module interne n'est pas renommé deux fois par le module externe.
- Table des noms par module (ticket 01) : indexée par chemin complet.

**Critère** : `a` contient `b` qui contient `c` ; `a.b.c.x` est le symbole.
`ENDMODULE` ferme le niveau le plus interne. Tests dans `tests/pp_test.cpp`.

## Réponse

- `keywords.cpp` : `MODULE`/`ENDMODULE` entrent dans `blockKinds()`. `end` ferme
  donc le module le plus interne, et `findMatching` apparie les modules.
- `pp.cpp` : handler `MODULE` réécrit. Préfixe composé `outer + mname + "."` ;
  `MODULE` sans fermeture, `ENDMODULE` orphelin, `MODULE` sans nom et `MODULE OFF`
  sont des erreurs nommées. `collectLabels(skipNestedModule)` suit une pile de
  blocs (un module fermé par `end` est bien vu). `renameModuleScope` : un module
  imbriqué garde ses propres noms (ils masquent ceux de l'extérieur), la ligne
  `MODULE x` n'est jamais renommée, les `@@export` internes ne sont pas retirés
  par l'extérieur. La table `moduleNames_` (ticket 01) est indexée par le chemin
  complet ; les blocs frères sont ceux du même niveau.
- Tests : `tests/pp_test.cpp` (imbrication 2 et 3 niveaux, masquage, visibilité,
  rouverture imbriquée, quatre erreurs), `tests/beautify_test.cpp` (le corps d'un
  module est désormais indenté, comme tout bloc). `make test` vert (14 suites).
- Doc : `docs/syntax.md` (section Modules), `CONTEXT.md`. `examples/struct_module.asm`
  assemble sans changement.

Limites connues : une rouverture ne voit que les blocs frères **antérieurs** pour
un module imbriqué (la table est alimentée au passage) ; une source qui écrivait
`MODULE a` / `MODULE b` sans `ENDMODULE` (forme commutateur) est maintenant refusée.
Reste pour le ticket 04 : commentaires de `pp.cpp` qui nomment l'autre assembleur.
