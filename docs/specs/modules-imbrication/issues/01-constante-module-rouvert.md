Status: resolved
Blocked by: —

# 01: Une constante définie dans un module est invisible à la réouverture du même module

**Symptôme** (reproduit) : `MODULE a` / `CONST EQU 123` / `ENDMODULE`, puis un
second `MODULE a` avec `LD A,CONST` → `unknown symbol 'CONST'`. `a.CONST` marche.

**Cause** : le renommage est textuel et par bloc (`pp.cpp:1554-1584`,
`collectLabels` sur `body`). Le second bloc ne définit pas `CONST`, donc rien
n'est renommé.

**À faire** : construire la table des noms par module sur **tous** les blocs
de ce module (pré-scan du fichier), et s'en servir pour renommer chaque bloc.
Le correctif doit représenter le module courant comme un chemin : le ticket 06
le réutilisera.

**Critère** : le cas ci-dessus assemble, et `a.CONST` reste la forme du symbole.
Ajouter un test à `tests/pp_test.cpp` près de la section MODULE (≈ l.520-540).

## Réponse

`pp.cpp` (handler MODULE) : les noms d'un module sont maintenant l'union des
noms de tous ses blocs `MODULE x` du fichier courant (pré-scan), stockée dans
`moduleNames_`, indexée par le chemin complet du module (`a.`). Chaque bloc est
renommé avec cette table. Test « MODULE rouvert : constante du premier bloc »
dans `tests/pp_test.cpp` ; suite complète verte (`make test`).

Limite connue, pour le ticket 06 : le pré-scan reste à plat (forme commutateur).
L'imbrication devra appairer les blocs et composer le chemin (`outer + mname + "."`).
