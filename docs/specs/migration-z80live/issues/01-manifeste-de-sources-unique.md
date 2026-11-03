# 01: Le manifeste de sources unique

**What to build:** la liste des modules du cœur cesse d'exister en plusieurs
exemplaires. Elle est déclarée **une seule fois**, dans un manifeste que les
trois points d'entrée de construction — le Makefile, la configuration CMake et
le script de construction WASM — **lisent** au lieu de la redéclarer. À la fin de
ce ticket, les trois construisent, et la construction WASM édite ses liens à
nouveau au lieu d'échouer sur symboles indéfinis.

C'est le préfactorage du chantier : il rend possible tout le reste, et il
transforme la faute qui a causé la panne en impossibilité structurelle plutôt
qu'en écart détectable.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Les modules du cœur sont énumérés à un seul endroit du dépôt
- [x] Le manifeste ne contient que le cœur ; chaque point d'entrée ajoute son
      propre module portant un `main`
- [x] Les trois points d'entrée lisent le manifeste ; aucun ne conserve de liste
      en dur
- [x] La construction native par le Makefile réussit
- [x] La configuration et la construction CMake réussissent, et la suite de tests
      existante passe intégralement
- [x] La construction WASM réussit — c'est le critère qui vérifie la réparation :
      elle échouait à l'édition de liens sur les quatre modules absents de sa
      liste
- [x] Ajouter un module au manifeste suffit à le faire entrer dans les trois
      constructions, sans autre édition

## Commentaires

Résolu. Le manifeste est `sources.manifest` à la racine : une ligne
« groupe fichier » par module du cœur, quinze au total. Le groupe nomme la
bibliothèque CMake qui porte le module ; les arêtes entre bibliothèques restent
dans la configuration CMake, parce qu'elles sont un fait de CMake et non une
liste de sources.

Les trois lecteurs :

- `Makefile` — `awk` sur la seconde colonne ; le dièse passe par une variable,
  make coupant sinon la ligne dessus ;
- `CMakeLists.txt` — `file(STRINGS ... ENCODING UTF-8)` puis une expression
  régulière par ligne, avec `CMAKE_CONFIGURE_DEPENDS` sur le manifeste pour que
  l'ajout d'un module reconfigure tout seul. Sans `ENCODING UTF-8`, CMake coupe
  les lignes sur les octets non ASCII et un fragment de commentaire accentué
  arrive sans son dièse ;
- `build-wasm.sh` — `mapfile` sur la même colonne, plus `asm_main.cpp` ; le
  manifeste est aussi entré dans le test de fraîcheur.

Vérifications passées : `make test` (onze suites unitaires et quatre tests
d'acceptation), `ctest` dans la distrobox (15/15), et la construction WASM, qui
échouait à l'édition de liens et produit désormais son `.mjs` et son `.wasm`.
Les listes lues par CMake et par le manifeste ont été comparées et sont
identiques.
