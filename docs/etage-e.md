# Étage E — suivi

> **Ce fichier tient lieu de ticket.** Une ligne d'état par étape. Le *quoi*
> est dans [spec-etage-e.md](spec-etage-e.md), la décision dans
> [adr/0032-…](adr/0032-le-port-est-exporte-par-le-profil-le-rang-d-un-axe-non.md).

**Ce qu'est l'étage E** : le second profil, CPC+, et le langage de profil
qu'il faut pour l'écrire sans compromis — `CONST`, et le retrait de l'axe
dans `__val_` (pas `__mask_`, corrigé en préparant E2 : voir l'amendement de
l'ADR 0032). **Ce qu'il n'est pas** : le builder, ni `.cpr` — ADR 0027, non
soldée ici.

## État

| # | Étape | Bloqué par | État |
|---|---|---|---|
| E0 | ADR 0032 : `CONST`, l'axe hors clé, le rang de `__port_` laissé ouvert | — | fait |
| E1 | `CONST <nom> = <expr>` : grammaire, analyseur, calcul CLI, `--dump-profile` | E0 | fait |
| E2 | Renommage `__val_<axe>_<clé>` → `__val_<clé>` (`__mask_` inchangé, corrigé en préparant E2) | E0 | fait |
| E3 | Profil `cpcplus` : fenêtres/banques RAM (copie du 6128), axe `cart_rom` sur `RMR2` | E0, E1 | fait |
| E4 | `CONST GA_PORT = 0x7F00` sur les deux profils CPC | E1, E3 | fait |
| E5 | Exemple d'acceptation : RAM identique au 6128, `cart_rom.w0<n>`/`w1<n>` calculent `%101 LRM n`, `--dump-profile cpcplus` se relit | E3, E4 | fait |
| E6 | `accept_const.sh` : un profil de test sans `CONST` continue d'émettre `__port_` | E1 | fait |
| E7 | Les dix suites (+ les nouvelles) vertes sur `Makefile` **et** `CMakeLists.txt` | E1–E6 | fait — 20/20 (18 + 2 sauts attendus WASM/épreuve) sur les deux listes, distrobox `ubuntu24-cross` |

## E1 — ce qui a été livré (TDD, seams confirmés en grilling)

- `profile.h`/`profile.cpp` : `Const { name, value, line }`, `Profile::consts`.
  `CONST <nom> = <expr>` — littéral + opérateurs, aucun `Name` (ADR 0032,
  round 1). Refusé : préfixe `__`, nom `CODE`/`PAGE`, doublon (même valeur ou
  pas), collision avec le paramètre d'un état d'axe — tous à l'analyse, tous
  nommant la ligne. `tests/profile_test.cpp` (seam 1), 8 nouveaux cas.
- `link.cpp::compute()` : `bind` est seedé avec `pr.consts` **avant**
  paramètre/`PAGE`/`CODE`, qui l'emportent s'ils se recouvrent (round 2) — un
  `SELECT` peut donc écrire `OUT GA_PORT` au lieu du port en dur.
  `switchSymbols` offre aussi chaque `CONST` tel quel au source, sans
  `EXTERN`, inconditionnellement (rien ne peut le rendre ambigu).
  `tests/link_test.cpp` (seam 2), 2 nouveaux cas.
- `tests/accept_const.sh` (seam 3, CLI réelle) : `GA_PORT` lu par le source
  ET par le `SELECT` qui l'écrit ; un profil sans `CONST` inchangé. Inscrit
  dans `Makefile` et `CMakeLists.txt`.
- `CONTEXT.md` : collision de vocabulaire trouvée et corrigée avant le code —
  le mot-clé s'appelait `EXPORT` dans les premiers jets, `CONTEXT.md` §Export
  réservait déjà ce mot au sens builder. Renommé en `CONST` partout ; entrée
  de glossaire ajoutée pour couper la confusion à la racine.

## E2 — ce qui a été livré (TDD ; une faille de l'ADR trouvée en préparant, avant tout code)

- **Amendement de l'ADR 0032, décision 2**, avant d'écrire une ligne : l'axe
  ne sort de `__mask_`/`__mask2_` — ils n'ont aucune clé de section
  (`offer("__mask_" + axisName, mask)`, rien d'autre), et le retirer aurait
  collapsé plusieurs masques distincts (déjà trois sur le seul CPC 6128) vers
  le même nom nu. Seul `__val_<axe>_<clé>` → `__val_<clé>` est renommé —
  `<clé>` est toujours un nom de section (unique, C1.0) ou un nom d'état déjà
  filtré à l'unicité par `named[...] == 1` avant d'atteindre `keys`.
- `link.cpp::compute()` : `offer("__val_" + k, val)` (l'axe disparaît de
  l'appel). Le sweep « un port sans sa valeur » comparait auparavant
  `__port_<axe>_<clé>` à `__val_<axe>_<clé>` par découpe de chaîne — cassé
  par le renommage, puisque `axisName` peut lui-même contenir un `_`
  (`rom_lower`). Remplacé par une table `mateOf` construite au moment où les
  deux moitiés sont encore connues séparément, plutôt qu'une reconstitution
  par sous-chaîne. `tests/link_test.cpp`, ~30 assertions renommées vers la
  nouvelle clé.
- Trouvé pendant l'implémentation, corrigé avant de committer : un commentaire
  ajouté à `link.cpp` nommait « CPC », faisant échouer `no_machine_names.sh` —
  reformulé sans nom de machine.
- Renommé partout où le vieux nom était un OCTET réel, pas seulement un test :
  `examples/banked.asm`, `aliased.asm`, `aliased_org.asm`, `aliased_sym.asm`,
  et les commentaires de `README.md`/`accept_banked.sh`/`accept_aliased.sh`.
- Vérifié sur les deux listes : `make test` (`Makefile`) et `ctest` dans la
  distrobox `ubuntu24-cross` (`CMakeLists.txt`) — 18/18 + 2 sauts attendus
  (WASM, épreuve snapshot) sur les deux.

## E3-E5 — le profil `cpcplus` (TDD ; profil écrit, puis validé par le seam 2)

- `profiles.cpp::kCpcPlus` : fenêtres/banques/`CONFIG SET ram`/`SELECT ram`
  **copiés à l'identique** du 6128 (§D.1 : même PAL, même table `ccc`) ;
  `rom_lower`/`rom_upper` copiés aussi (`RMR` inchangé sur Plus). `CONST
  GA_PORT = 0x7F00` déclaré une fois, utilisé par les quatre `SELECT`.
- L'axe propre au Plus, `cart_rom` (nom définitif — `lrom2` était provisoire) :
  `RMR2`, sélectionné par les bits 7-5 = `101`, encode en un seul octet la
  fenêtre (`LRM`, bits 4-3) ET l'ID de ROM physique de la cartouche (bits
  2-0, 0..7 seulement — §D.1 point 3). Trois états paramétriques
  (`w0<n>`/`w1<n>`/`w2<n>`), chacun `CODE = (LRM << 3) | n`, vérifiés contre
  les deux exemples littéraux de [GRIM-GA] (`&7FA0`, `&7FB8`).
  `BANK crom<n> STORE 16` : un numéro déplacé hors de 0..9 (déjà pris), pas
  une valeur matérielle — l'ID physique attesté reste `PAGE`, comme
  `rom_hi<n>` (C1.8).
- **Hors périmètre, nommé dans le profil et non subi** : `LRM = 11` (ROM +
  page E/S ASIC en même temps — une page d'E/S n'est pas une banque
  adressable par une SECTION, demanderait un mot de profil neuf) ; les ROM
  physiques 8..31 (adressables seulement en ROM haute, `&DF00`) ; le
  déverrouillage ASIC (état d'exécution, jamais représentable comme un
  `SELECT` — ADR 0032).
- `tests/link_test.cpp` : le profil livré `cpcplus` se lit, `GA_PORT` s'y
  résout, la RAM y calcule les mêmes octets qu'au 6128, et `cart_rom.w0<3>`/
  `w1<3>` calculent `&A3`/`&AB` — déplacer la fenêtre change le `CODE`, pas
  le `n`. `Makefile`/`CMakeLists.txt` durent aussi lier `profiles.cpp` à
  `link_test` (manquait, jamais nécessaire avant que le test exerce un
  profil livré).
- Vérifié sur les deux listes (20/20, `make test` et `ctest` dans
  `ubuntu24-cross`).

## Notes de suivi

*(à tenir à jour au fil de l'implémentation — une ligne par décision prise en
cours de route, comme `docs/etage-c1.md` §C1.7 « Deux corrections, après
usage ».)*
