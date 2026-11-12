# Améliorations et questions en suspens

## Améliorations

- **Autres découpages de l'ADR 0007** : le DSK ne connaît qu'un découpage, un morceau par emplacement d'un seul tenant (blocs contigus réunis dans les 64 K de base comme dans une même banque d'extension). Le morceau englobant, le découpage par banque de 16 K et le découpage par groupe de 64 K, ainsi qu'une option `--split=…` pour choisir, attendent un besoin réel.
- **Nom AMSDOS 8.3 en double** (`pack.cpp`, `amsdos`, `Volume::erase`, `Volume::write`) : le remplissage du nom sur 8 et de l'extension sur 3 est copié trois fois, et le couple `(name, ext)` voyage ensemble dans `amsdos`, `erase` et `write`. Un petit type « nom AMSDOS » porterait les deux.
- **Ajouter un fichier quelconque au DSK** (`pack.cpp`, `asm_main.cpp`) : `-o out.dsk` n'y met que le programme lié. Un projet qui livre aussi un fichier tel quel (BASIC ASCII sans en-tête, données) passe par un script à côté : `amspirit-core/tools/fdc-judge/dsk_putfile.py` pour `COPIE.BAS`. Une option du type `--dsk-file chemin[:NOM.EXT]` (sans en-tête AMSDOS) le remplacerait.
- **Binaire AMSDOS** (`pack.cpp`) : un `.bin` avec l'en-tête AMSDOS de 128 octets, hors disquette. L'encapsulation existe déjà dans `pack` pour le DSK ; il manque une destination.

## Questions à trancher

- **Accents dans les messages** (`pack.cpp`) : un message est sans accents (« entree », « a 0 ») quand les autres du fichier en ont ; le dépôt mélange les deux sans convention écrite.
- **Disquette de 41 ou 42 pistes** (`pack.cpp`, chargement d'une disquette existante) : elle est refusée, conformément à la lettre de la spec. Faut-il tolérer les pistes en trop d'une disquette DATA ?
- **`.cpr` d'une image qui écrit hors de `crom`** (`cpr.cpp`, `bankBytes`) : le CPR ne lit que la banque `crom` et ignore sans rien dire ce que l'image écrit ailleurs (RAM, `rom_lo`…), là où le CRO refuse une image qui remplit plusieurs banques de ROM. Faut-il avertir, ou refuser ?

- **`PP equ $` en section relocalisable** (`asm.cpp`, `noteSymbol`) : exporté dans le `.fo` comme constante (`section=""`, valeur `0xF`), pas comme label rattaché à la section, alors que `syntax.md:388` dit que `$` se comporte comme un label. Un autre module qui ferait `extern PP` verrait une constante. Non comparé à l'état d'avant `cf8618e` ; à reproduire avant de trancher.

## Bugs à vérifier

- **Avertissement parasite sans `run`** (`pack.cpp`, `link.cpp` défaut de `runAddress`) : hypothèse non reproduite. Un programme sans `run` qui n'écrit qu'en banque d'extension a `runAddress` = `loadAddress` = 0, et produirait « le run &0000 ne tombe dans aucun fichier », alors que le défaut doit marcher (US 5 de la spec DSK).
- **Morceau de 64 K pile** (`pack.cpp`, en-tête AMSDOS) : hypothèse. La longueur sur 16 bits vaudrait 0.
