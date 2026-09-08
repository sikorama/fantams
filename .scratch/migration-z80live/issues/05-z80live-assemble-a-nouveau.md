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

**Status:** ready-for-agent

- [ ] L'artefact WASM est reconstruit depuis les sources courantes, dans
      l'environnement qui dispose de l'outillage de compilation
- [ ] Le test de fraîcheur du script de construction ne se déclenche plus
- [ ] Le pointeur de sous-module du dépôt consommateur est avancé par un commit
      qui l'énonce ; l'arbre de travail et le pointeur enregistré concordent
- [ ] Les artefacts de construction ne sont pas versionnés dans fantams : ce sont
      des produits
- [ ] z80live assemble une source dans le navigateur et rend un snapshot
- [ ] La source déroulée et les diagnostics remontent toujours dans l'interface
- [ ] Aucune option de l'adaptateur CLI employée par le dépôt consommateur n'a
      eu besoin d'être changée — si l'une l'a été, c'est un écart à signaler, pas
      à corriger en silence
