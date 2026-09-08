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

**Status:** ready-for-agent

- [ ] Le test vit dans fantams, et non dans le dépôt consommateur : c'est fantams
      qui garantit la justesse de son propre adaptateur
- [ ] Il est câblé au lanceur de tests selon le moule des tests d'acceptation
      existants — script shell, répertoire de travail à la racine, chemin du
      binaire construit passé par l'environnement
- [ ] Sur au moins un cas de référence, les octets produits par les deux
      adaptateurs sont identiques
- [ ] Le test se saute — et non échoue — quand l'artefact WASM est absent
- [ ] Le test se saute — et non échoue — quand l'exécutable node est absent
- [ ] Un saut est visible comme tel dans le rapport de la suite, distinct d'un
      succès
- [ ] Confronté à l'artefact WASM périmé actuellement livré, le test échoue :
      c'est la démonstration qu'il attrape la faute qu'on répare
