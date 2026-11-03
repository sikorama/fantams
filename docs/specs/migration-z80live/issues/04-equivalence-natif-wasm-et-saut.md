# 04: L'équivalence natif ≡ WASM, avec la discipline de saut

**What to build:** le verrou du chantier. Un test de la suite vérifie que le
**même `argv` et les mêmes fichiers, à travers l'adaptateur natif et à travers
l'adaptateur WASM, rendent les mêmes octets**.

C'est la propriété que la couture rend disponible gratuitement — l'adaptateur
WASM n'exporte aucune fonction du cœur, son contrat *est* le contrat CLI — et
c'est celle qu'un artefact périmé viole. Elle remplace le critère en place, qui
se contentait de vérifier la signature du snapshot et un drapeau de succès, et
qu'un artefact vieux de trois étages satisfait sans difficulté.

Ce ticket installe aussi la **discipline de saut** dont le reste du chantier
dépendra : un test dont la dépendance externe est absente se **saute** au lieu
d'échouer, par le mécanisme prévu à cet effet par le lanceur de tests. Un saut
doit rester bruyant dans le rapport — un test sauté n'est pas un test réussi, et
tout le verrou repose sur cette distinction.

**Blocked by:** 01 (le manifeste de sources unique).

**Status:** resolved

- [x] Le test vit dans fantams, et non dans le dépôt consommateur : c'est fantams
      qui garantit la justesse de son propre adaptateur
- [x] Il est câblé au lanceur de tests selon le moule des tests d'acceptation
      existants — script shell, répertoire de travail à la racine, chemin du
      binaire construit passé par l'environnement
- [x] Sur au moins un cas de référence, les octets produits par les deux
      adaptateurs sont identiques
- [x] Le test se saute — et non échoue — quand l'artefact WASM est absent
- [x] Le test se saute — et non échoue — quand l'exécutable node est absent
- [x] Un saut est visible comme tel dans le rapport de la suite, distinct d'un
      succès
- [x] Confronté à l'artefact WASM périmé actuellement livré, le test échoue :
      c'est la démonstration qu'il attrape la faute qu'on répare

## Commentaires

Résolu. `tests/accept_wasm_equiv.sh`, sur le moule exact des quatre tests
d'acceptation existants — script shell sous `tests/`, répertoire de travail à la
racine, chemin du binaire construit passé par `FANTAMS`. Il s'appuie sur
`tests/wasm_cli.mjs`, un pilote qui ne fait rien d'autre que poser les fichiers
d'entrée dans le système de fichiers virtuel, appeler `callMain` avec l'argv
qu'on lui donne, et ressortir les fichiers demandés : l'adaptateur WASM
n'exportant aucune fonction du cœur, son contrat *est* le contrat CLI.

Cinq cas, tous à argv identique des deux côtés :

- `demo.asm` -> `.bin`, 14 octets identiques ;
- `demo.asm` -> `.sna`, 65 792 octets identiques ;
- `demo.asm -E`, la source déroulée — du texte, où une divergence de
  préprocesseur se verrait ;
- `banked.asm -T banked.ld --target cpc6128` — les capacités de l'étage C1,
  c'est-à-dire précisément les quatre modules qui manquaient à la liste ;
- `--version`, même forme et même date de version des deux côtés.

**Discipline de saut.** Le script rend 77 quand l'artefact WASM ou node
manquent. CMake le déclare par `SKIP_RETURN_CODE 77` ; ctest écrit alors
`***Skipped` et, en fin de rapport, « The following tests did not run ». Le
`make test` fait le même partage et dit à voix haute que la suite n'a *pas*
vérifié l'équivalence. Un saut n'est pas un succès.

**Le critère qui compte.** Confronté à l'artefact WASM périmé actuellement livré
au dépôt consommateur, le test échoue — le cas banque est refusé, et `--version`
n'existe pas dans cet artefact :

    ECHEC : l'adaptateur WASM refuse le cas banque (etage C1 absent de l'artefact ?)
    ECHEC : date de version natif=2026-09-08 wasm=file — l'artefact WASM est perime

Les trois premiers cas passent pourtant contre cet artefact : c'est exactement
ce que l'ancien critère — signature du snapshot plus drapeau de succès —
regardait, et pourquoi il n'a rien vu pendant trois étages.
