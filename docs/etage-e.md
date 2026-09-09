# Étage E — suivi

> **Ce fichier tient lieu de ticket.** Une ligne d'état par étape. Le *quoi*
> est dans [spec-etage-e.md](spec-etage-e.md), la décision dans
> [adr/0032-…](adr/0032-le-port-est-exporte-par-le-profil-le-rang-d-un-axe-non.md).

**Ce qu'est l'étage E** : le second profil, CPC+, et le langage de profil
qu'il faut pour l'écrire sans compromis — `CONST`, et le retrait de l'axe
dans `__val_`/`__mask_`. **Ce qu'il n'est pas** : le builder, ni `.cpr` — ADR
0027, non soldée ici.

## État

| # | Étape | Bloqué par | État |
|---|---|---|---|
| E0 | ADR 0032 : `CONST`, l'axe hors clé, le rang de `__port_` laissé ouvert | — | fait |
| E1 | `CONST <nom> = <expr>` : grammaire, analyseur, calcul CLI, `--dump-profile` | E0 | fait |
| E2 | Renommage `__val_<axe>_<clé>` → `__val_<clé>` / `__mask_<axe>` → `__mask` | E0 | à faire |
| E3 | Profil `cpcplus` : fenêtres/banques RAM (copie du 6128), axe `lrom2` sur `RMR2` | E0, E1 | à faire |
| E4 | `CONST GA_PORT = 0x7F00` sur les deux profils CPC ; `__port_` retiré des exemples qui l'utilisaient | E1, E3 | à faire |
| E5 | Exemple d'acceptation : section `ram` + section `ro` sous `lrom2`, `--dump-profile cpcplus`, déplacement de `lrom2.mid`→`lrom2.high` sans adresse logique touchée | E3, E4 | à faire |
| E6 | `accept_const.sh` : un profil de test sans `CONST` continue d'émettre `__port_` | E1 | fait |
| E7 | Les dix suites (+ les nouvelles) vertes sur `Makefile` **et** `CMakeLists.txt` | E1–E6 | E1/E6 verts sur les deux listes ; le reste attend E2-E5 |

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

## Notes de suivi

*(à tenir à jour au fil de l'implémentation — une ligne par décision prise en
cours de route, comme `docs/etage-c1.md` §C1.7 « Deux corrections, après
usage ».)*
