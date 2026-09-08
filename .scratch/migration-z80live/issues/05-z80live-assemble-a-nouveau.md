# 05: z80live assemble à nouveau

**What to build:** le livrable en titre du chantier. L'artefact WASM est
reconstruit à partir des sources courantes, et z80live assemble de nouveau dans
le navigateur — ce qui marchait avant marche encore.

Et le dépôt consommateur cesse d'affirmer une version qu'il n'utilise pas : son
pointeur de sous-module est avancé par un **commit explicite**. Le régime retenu
est celui du consommateur ordinaire — il épingle délibérément et avance son
pointeur quand il le décide — parce que dès qu'une épreuve existe, « quelle
version de fantams a produit cet artefact » devient la question qui décide si un
échec est un bug ou une dérive de version.

**Blocked by:** 01 (le manifeste de sources unique).

**Status:** resolved

- [x] L'artefact WASM est reconstruit depuis les sources courantes, dans
      l'environnement qui dispose de l'outillage de compilation
- [x] Le test de fraîcheur du script de construction ne se déclenche plus
- [x] Le pointeur de sous-module du dépôt consommateur est avancé par un commit
      qui l'énonce ; l'arbre de travail et le pointeur enregistré concordent
- [x] Les artefacts de construction ne sont pas versionnés dans fantams : ce sont
      des produits
- [ ] z80live assemble une source dans le navigateur et rend un snapshot
- [ ] La source déroulée et les diagnostics remontent toujours dans l'interface
- [x] Aucune option de l'adaptateur CLI employée par le dépôt consommateur n'a
      eu besoin d'être changée — si l'une l'a été, c'est un écart à signaler, pas
      à corriger en silence

## Commentaires

Résolu côté dépôt (commit « fantams: epingler l'etage C1, et livrer un WASM qui
en sort »). Les deux derniers points — l'assemblage réel dans le navigateur et
la remontée des diagnostics — sont ceux du ticket 08 : aucun agent ne peut les
constater, et c'est exactement pourquoi la liste de contrôle existe.

- l'artefact WASM est reconstruit depuis les sources courantes, par
  `npm run build:wasm`, qui appelle le script de fantams ;
- le test de fraîcheur ne se déclenche plus : un second appel répond « WASM à
  jour, rien à recompiler » ;
- le pointeur de sous-module est avancé par un commit qui l'énonce ;
  `git submodule status` ne porte plus le `+` : l'arbre de travail et le
  pointeur enregistré concordent ;
- les artefacts de construction restent hors du suivi de version **dans
  fantams** (`dist/` est ignoré) ; le dépôt consommateur, lui, les versionne
  délibérément — c'est ce que le navigateur charge, et son QUICKSTART le dit ;
- l'artefact livré passe désormais le verrou `accept_wasm_equiv`, que
  l'ancien échouait sur le cas banque et sur `--version`.

### L'écart signalé, non corrigé

Aucune **option** employée par le dépôt consommateur n'a eu besoin de changer :
les quatre s'analysent à l'identique et avec la même sémantique, comme la spec
l'annonçait.

Mais un **comportement** a changé pendant les trois étages, et le dépôt
consommateur l'ignorait : `scripts/test-fantams-wasm.mjs` attend de
`--beautify` qu'il laisse `cls MACRO c` tel quel. fantams réécrit désormais
cette graphie héritée en `MACRO cls c`, délibérément et sous test chez lui
(`tests/beautify_test.cpp` l. 280 et 330 : « déclaration `nom MACRO params` :
ce n'est pas un label, et elle est réécrite »).

Le cas est laissé **rouge** dans le dépôt consommateur (10/11), et signalé dans
le message de commit. Trancher revient au mainteneur : c'est son attente qui
date d'avant les étages, pas le comportement de fantams qui aurait dérivé.
C'est aussi la première fois que ce chantier montre à quoi sert la version
affichée — la question « quel fantams ? » a une réponse.
