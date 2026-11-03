# Améliorations et questions en suspens

## Améliorations

- **Faire passer `sna`, `cpr` et `cro` par la couche `pack`** (`sna.cpp`, `cpr.cpp`, `cro.cpp`, `asm_main.cpp`) : l'export DSK pose la couche `pack` de l'ADR 0007 avec le DSK pour seul client. Les trois formats existants gardent leur code propre et leur aiguillage sur l'extension dans `asm_main.cpp`.
- **Autres découpages de l'ADR 0007** : le DSK ne connaît qu'un découpage, un morceau par emplacement d'un seul tenant (blocs contigus réunis dans les 64 K de base comme dans une même banque d'extension). Le morceau englobant, le découpage par banque de 16 K et le découpage par groupe de 64 K, ainsi qu'une option `--split=…` pour choisir, attendent un besoin réel.
- **Nom AMSDOS 8.3 en double** (`pack.cpp:86-87`, `186-187`, `233-234`) : le remplissage du nom sur 8 et de l'extension sur 3 est copié trois fois, et le couple `(name, ext)` voyage ensemble dans `amsdos`, `erase` et `write`. Un petit type « nom AMSDOS » porterait les deux.
- **Lecture de la disquette existante** (`asm_main.cpp:747-756`) : troisième copie du bloc de lecture des branches cpr/cro, alors que `readFile` existe (`asm_main.cpp:93`). Le retrait chemin + extension de `pack.cpp:59-62` double aussi `asm_main.cpp:725-727`.
- **`Result::artifacts`** (`pack.h`, `asm_main.cpp:762`) : l'appelant prend `.front()` sans vérifier que la liste n'est pas vide.
- **Ajouter un fichier quelconque au DSK** (`pack.cpp`, `asm_main.cpp`) : `-o out.dsk` n'y met que le programme lié. Un projet qui livre aussi un fichier tel quel (BASIC ASCII sans en-tête, données) passe par un script à côté : `amspirit-core/tools/fdc-judge/dsk_putfile.py` pour `COPIE.BAS`. Une option du type `--dsk-file chemin[:NOM.EXT]` (sans en-tête AMSDOS) le remplacerait.

## Questions à trancher

- **Nom du module `pack`** : `CONTEXT.md` classe « packer » parmi les termes à éviter, et « builder » n'y a pas d'entrée. `amsdos()` (`pack.cpp`) nomme le format plutôt que l'**Encapsulation**.
- **Accents dans les messages** (`pack.cpp`) : un message est sans accents (« entree », « a 0 ») quand les autres du fichier en ont ; le dépôt mélange les deux sans convention écrite.
- **Disquette de 41 ou 42 pistes** (`pack.cpp`, chargement d'une disquette existante) : elle est refusée, conformément à la lettre de la spec. Faut-il tolérer les pistes en trop d'une disquette DATA ?

## Bugs à vérifier

- **Avertissement parasite sans `run`** (`pack.cpp`, `link.cpp` défaut de `runAddress`) : hypothèse non reproduite. Un programme sans `run` qui n'écrit qu'en banque d'extension a `runAddress` = `loadAddress` = 0, et produirait « le run &0000 ne tombe dans aucun fichier », alors que le défaut doit marcher (US 5 de la spec DSK).
- **Morceau de 64 K pile** (`pack.cpp`, en-tête AMSDOS) : hypothèse. La longueur sur 16 bits vaudrait 0.
