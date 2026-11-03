# Spec — Export DSK : livrer un programme sur disquette AMSDOS

Status: ready-for-agent

## Problem Statement

fantams sait produire un binaire brut, un snapshot (`.sna`), une cartouche
(`.cpr`) et un conteneur de ROMs (`.cro`). Il ne sait pas produire de
disquette. Or une disquette est le support de distribution courant d'un
programme CPC, et le seul qui marche sur une machine réelle sans carte
d'extension.

Aujourd'hui, l'auteur doit sortir un binaire brut, le passer à un outil externe
qui ajoute l'en-tête AMSDOS et le pose sur un `.dsk`, puis recommencer à chaque
build. Pour un programme en banques, c'est pire : il doit découper l'image à la
main, une plage par banque, et recalculer chaque adresse de chargement que le
linker connaissait déjà.

Le builder de la chaîne (étage 3 de `docs/spec-chaine-outils.md`) n'existe pas
encore : chaque format de sortie a son code à part, sans découpage ni
encapsulation partagés. L'ADR 0007 décrit la forme visée et attendait justement
un format à plusieurs fichiers pour la poser.

## Solution

`fantams prog.asm -o prog.dsk` produit une disquette AMSDOS directement
utilisable en émulateur et sur machine réelle.

- Un programme d'un seul tenant devient **un fichier**, nommé d'après le source
  (`PROG.BIN`), avec un en-tête AMSDOS qui porte son adresse de chargement et son
  point d'entrée : `RUN"PROG"` le lance.
- Un programme en banques devient **un fichier par morceau**, nommé d'après son
  emplacement (`B05_4000.BIN` : banque 5, chargé en `&4000`). L'auteur écrit son
  loader, et le nom de chaque fichier lui dit où le charger.
- Si la disquette existe déjà, les fichiers produits y sont **ajoutés ou
  remplacés par nom**. Ce qui s'y trouvait par ailleurs, un loader BASIC posé à
  la main par exemple, est conservé.

Le format est choisi d'après l'extension de `-o`, comme pour `.sna`, `.cpr` et
`.cro`. Sous le capot, l'export passe par une nouvelle couche `pack`
(découpage → morceaux → encapsulation → conteneur → artefacts, ADR 0007), dont
le DSK est le premier client.

## User Stories

1. As an author of a small CPC program, I want `-o prog.dsk` to produce a disk image, so that I can test my program in an emulator the way it will be distributed.
2. As an author, I want my single-block program to land on the disk as one file named after my source, so that I can type `RUN"PROG"` without remembering an address-based name.
3. As an author, I want the AMSDOS header to carry the load address of my code, so that `LOAD"PROG"` puts it where the linker placed it.
4. As an author, I want the AMSDOS header to carry my `run` address as entry point, so that `RUN"PROG"` starts my program without a BASIC stub.
5. As an author without a `run` directive, I want the entry point to default to the load address, so that the common one-`org` case keeps working.
6. As an author of a program spanning `&4000`–`&9000` in the base 64 K, I want it delivered as one file, so that a 16 K boundary inside my code does not split it in two.
7. As an author of two disjoint `org` blocks in the base 64 K, I want two files, so that the gap between them is not padded onto the disk.
8. As an author of a banked program, I want one file per contiguous run of an extension bank, so that my loader can page each bank in before loading its file.
9. As an author of a banked program, I want each file named `Bxx_yyyy.BIN` from its bank and load address, so that the name tells my loader where it goes.
10. As an author of a banked program, I want a file loaded in bank 5 through window `&4000` to carry `&4000` as load address, so that `LOAD` with the bank paged in puts it in the right place.
11. As an author, I want only the file containing my `run` address to carry it as entry point, and the others to carry 0, so that no data file pretends to be executable.
12. As an author using `org logical,store`, I want a warning when my `run` address falls in no file on the disk, so that I know the entry points were left at 0 and my bootstrap must jump there itself.
13. As an author, I want reserved holes (`ds`) inside a block written as zeros, so that the file covers exactly what the linker placed and a buffer at the end of a block is cleared on load.
14. As an author, I want a hand-made BASIC loader already on the disk to survive a rebuild, so that I can keep `DISC.BAS` on the same disk as my program.
15. As an author, I want rebuilding to replace my previous `PROG.BIN` rather than add a second one, so that the disk always holds the current build.
16. As an author, I want name comparison to ignore case, so that a file written by another tool as `prog.bin` is replaced too.
17. As an author, I want to add my program to an existing disk in standard (MV - CPCEMU) format, so that disks produced by other tools are usable.
18. As an author, I want the output always written in EXTENDED format, so that the result is read by every current emulator.
19. As an author, I want a clear error when the existing disk is not in DATA format (SYSTEM, VENDOR, double-sided, protected), so that fantams never corrupts a disk it does not understand.
20. As an author, I want the existing disk left untouched when the export fails, so that a failed build does not destroy my disk.
21. As an author, I want a clear error when the disk is full, in space or in directory entries, so that I am not handed a disk with missing files.
22. As an author, I want `--base`, `--cpr-bank` and `--cro-*` refused with a `.dsk` output, so that options that mean nothing for a disk are not silently ignored.
23. As an author, I want `--sym` and `--timing` to work with a `.dsk` output, so that I keep my debugging tools whatever the output format.
24. As an author whose source name contains characters AMSDOS does not accept, I want them dropped and the name upper-cased and cut to 8 characters, so that the disk stays readable.
25. As an author whose source name keeps no valid character, I want the derived `Bxx_yyyy` name instead, so that the export never fails on a name.
26. As an author linking several `.fo` objects, I want the single file named after the first input, so that the name is predictable.
27. As an author of a banked program, I want fantams not to generate a loader, so that the paging code stays mine and in my source, as the toolchain spec requires.
28. As a maintainer, I want the DSK built from a shared splitting and encapsulation layer, so that a future loose AMSDOS binary or another format does not reimplement them.
29. As a maintainer, I want the `pack` layer to take the existing disk as data rather than reading files, so that it is tested in-process without touching the filesystem.

## Implementation Decisions

**Couche `pack` (ADR 0007).** Un nouveau module porte l'interface du builder
décrite au §2.3 de `docs/coutures-de-la-chaine.md` : `package(image, container)`
rend un ensemble d'artefacts nommés (chemin et octets). Le DSK est son seul
client. `sna`, `cpr` et `cro` ne changent pas (migration notée dans
`improvements.md`).

- Le **conteneur** DSK porte comme données : le nom du source principal (pour
  nommer un morceau unique) et les octets de la disquette existante (vides s'il
  n'y en a pas). `package` ne lit ni n'écrit aucun fichier.
- `package` rend aussi des **diagnostics** : erreurs (disquette pleine, disquette
  existante illisible ou hors format DATA) et avertissements (`run` hors de tout
  morceau). Ils passent par le même canal `asmb::Diagnostic` que l'`Image`. En
  cas d'erreur, aucun artefact.
- Pour un DSK, le résultat est **un artefact** : l'image disque entière. La CLI
  l'écrit à l'emplacement de `-o`.

**Découpage.** Un morceau par emplacement d'un seul tenant. Le linker pose un
bloc par fragment, et coupe en plus un fragment des 64 K de base à chaque
frontière de 16 K (la banque y suit l'adresse). Les blocs sont triés par
emplacement, puis deux blocs sont fusionnés en un seul morceau quand les deux
conditions suivantes sont réunies :

- ils sont dans le même espace : la même banque d'extension, ou tous deux dans
  les 64 K de base à leur fenêtre naturelle (banque = adresse >> 14) ;
- leurs adresses de rangement se suivent.

Aucune option de découpage n'est exposée. Les sections `uninit` ne produisent
rien, puisqu'elles ne sont pas dans les blocs. `examples/banked.asm` donne trois
fichiers : `B01_4000`, `B02_8000` et `B05_4000` (`audio` et `packed`
réunis).

**Nommage.**

- Un seul morceau : nom du premier fichier d'entrée, sans extension, en
  majuscules, limité à `A-Z 0-9 _ -`, tronqué à 8 caractères, avec l'extension
  `.BIN`. Si rien ne reste après filtrage, on prend le nom dérivé.
- Plusieurs morceaux : nom dérivé `B` + banque en 2 chiffres hexa + `_` +
  adresse de chargement en 4 chiffres hexa, avec l'extension `.BIN`. Ce sont
  la banque et l'adresse du premier bloc du morceau.

**Encapsulation AMSDOS.** En-tête de 128 octets :

- type binaire (2), user 0 ;
- adresse de chargement = adresse de rangement du morceau dans sa fenêtre (16
  bits, comme `Block::addr`) ;
- longueur ;
- point d'entrée : `runAddress` de l'`Image` si elle tombe dans l'intervalle de
  rangement du morceau et que `runBank` (la banque de rangement du label que
  `run` nomme) est dans l'espace du morceau, 0 sinon. L'adresse seule ne
  distingue pas `&4000` dans les 64 K de base de `&4000` en banque d'extension ;
- somme de contrôle standard sur les 67 premiers octets.

Si `runAddress` ne tombe dans aucun morceau, tous les points d'entrée sont à 0
et `package` émet un avertissement qui nomme l'adresse. Les trous réservés sont
écrits à zéro, sans rognage.

**Conteneur DSK.**

- En sortie : EXTENDED CPC DSK, format DATA (40 pistes, une face, 9 secteurs de
  512 octets numérotés `&C1`–`&C9`, blocs de 1 K, 2 blocs de répertoire, 64
  entrées), soit 178 K utiles.
- Disquette existante : en entrée, on accepte le format standard (MV - CPCEMU)
  ou étendu, à condition qu'il soit en DATA. Tout autre format est refusé.
- Fusion : un fichier produit remplace celui de même nom et de même extension
  sur le user 0 (comparaison sans tenir compte de la casse). Les blocs et les
  entrées de l'ancien fichier sont libérés avant l'écriture. Les autres fichiers
  sont conservés, octet pour octet.
- Disquette pleine (blocs ou entrées de répertoire) : erreur, sans artefact.

**CLI.**

- `-o x.dsk` choisit la sortie DSK d'après l'extension.
- `--base`, `--cpr-bank` et `--cro-*` sont refusés avec un `.dsk`, avant
  l'assemblage, avec le même genre de message que pour les autres formats.
- `--sym` et `--timing` restent acceptés.
- La CLI lit le `.dsk` existant s'il y en a un, appelle `package`, affiche les
  diagnostics et n'écrit le fichier que s'il n'y a pas d'erreur.
- L'usage (`--help` et message d'usage) mentionne `.dsk`.

**Documentation à remettre à l'état actuel pendant l'implémentation.**

- ADR 0007 : passer à `accepted` et le corriger sur place.
  - Remplacer l'encadré « le pipeline n'est pas écrit ».
  - Décrire le découpage retenu et dire que les autres découpages attendent un
    besoin.
  - Ajouter la règle du morceau unique nommé d'après le source.
  - Retirer le paragraphe SNA dépassé par l'ADR 0006, en gardant le piège RLE.
- `CONTEXT.md` : ligne **Backend** (« plus tard `dsk` »).
- `docs/coutures-de-la-chaine.md` : encadré d'en-tête (« le builder reste à
  écrire ») et §2.3 si le contrat retenu diffère (conteneur porteur de la
  disquette existante, diagnostics).
- `docs/spec-chaine-outils.md` : en-tête (« le builder ne l'est pas »).
- `README.md` : section des formats de sortie, et signature des backends DSK.

## Testing Decisions

Un bon test vérifie le comportement externe : il produit une `Image` comme le
ferait le linker et lit le `.dsk` comme le ferait un émulateur (répertoire,
en-têtes AMSDOS, octets). Il ne teste ni le découpage ni l'encapsulation à
travers des fonctions internes.

**Tests unitaires de `package`** (`pack_test`), sur une vraie `link::Image`
construite par `link::build` à partir d'objets et d'un script, avec un petit
lecteur DSK écrit dans le test. Ils couvrent :

- un `org` unique : un fichier nommé d'après le source, avec l'adresse de
  chargement et le point d'entrée attendus et une somme de contrôle juste ;
- un programme de `&4000` à `&9000` : un seul fichier ;
- deux `org` disjoints : deux fichiers aux noms dérivés ;
- un bloc en banque d'extension : un fichier `B05_4000.BIN` chargé en `&4000` ;
- un point d'entrée porté par le seul morceau qui contient le `run` ;
- un `run` hors de tout morceau : avertissement et points d'entrée à 0 ;
- un trou réservé écrit à zéro ;
- un nom de source filtré, et un nom vide qui donne le nom dérivé ;
- la fusion avec une disquette existante, standard et étendue : remplacement
  par nom sans tenir compte de la casse, autres fichiers conservés ;
- le refus d'une disquette hors format DATA ;
- la disquette pleine, par la place et par les entrées de répertoire.

Antériorité : `cpr_test` et `cro_test`, qui construisent une `Image` par
`link::build` et décodent le conteneur produit.

**Test d'acceptation de la CLI** (`accept_dsk.sh`). Il couvre :

- `examples/demo.asm` et `examples/banked.asm` de bout en bout ;
- deux invocations sur le même `.dsk`, qui laissent les deux programmes ;
- le refus de `--base`, `--cpr-bank` et `--cro-*` ;
- `--sym` accepté ;
- une disquette existante inchangée après un export en erreur.

Antériorité : `accept_cro.sh`. Le nouveau test et le nouveau script rejoignent
`make test-fast` et `make test` comme les autres, et le nouveau module entre
dans `sources.manifest`.

**Contrôle manuel, hors dépôt** : charger et lancer les deux `.dsk` d'exemple
dans AMSpiriT.

## Out of Scope

- Génération de loader (BASIC ou Z80) : exclue par la spec de la chaîne, §12.4.
- Option de découpage (`--split`) et autres découpages de l'ADR 0007 (morceau
  englobant, par banque de 16 K, par groupe de 64 K).
- Migration de `sna`, `cpr` et `cro` vers la couche `pack`.
- Binaire AMSDOS seul hors disquette (`.bin` préfixé).
- Formats SYSTEM et VENDOR, double face, protections, plus de 40 pistes ; CDT
  (cassette).
- Fichiers en lecture seule ou cachés, users autres que 0, autres types AMSDOS
  que binaire.
- Nettoyage des fichiers `Bxx_yyyy.BIN` d'un build précédent dont le découpage a
  changé : la fusion par nom les laisse sur la disquette, et la documentation le
  dit.
- Collisions de noms entre plusieurs programmes en banques sur une même
  disquette (l'ADR 0007 réserve l'extension pour cela).

## Further Notes

- Le cadrage a montré que « un morceau par bloc » seul aurait coupé en deux un
  programme d'un seul tenant de `&4000` à `&9000` : d'où la règle de fusion dans
  les 64 K de base. L'implémentation a montré le même défaut dans une banque
  d'extension (`audio` et `packed` de `examples/banked.asm`, deux fichiers pour
  16 octets) : la fusion vaut donc aussi dans une même banque.
- Le nommage d'un morceau unique d'après le source amende l'ADR 0007, qui
  dérivait toujours le nom de l'emplacement ; `CONTEXT.md` (**Morceau**) porte
  déjà la règle.
- Le point d'entrée est une adresse logique et l'adresse de chargement une
  adresse de rangement : dans un bloc `org logique,rangement`, elles peuvent ne
  pas se recouper, d'où l'avertissement plutôt qu'une erreur.
