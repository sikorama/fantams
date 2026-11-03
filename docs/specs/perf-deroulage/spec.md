# Chantier : performance des sources déroulées longues

Status: needs-triage

Les sources à boucles imbriquées (`repeat`/`for` + `if` par tour) étaient lentes :
4 à 5 s dans z80live (WASM), 1 s en natif sur `tests/bench/nested_if.asm`. Le
coût était **par ligne déroulée**, pas arithmétique. Décisions : ADR 0034,
glossaire « Source déroulée pliée ».

## Fait (2026-10-02)

| Étape | Commit | `nested_if` natif |
|---|---|---|
| départ | `de199ac` | 1 048 ms |
| tables de hachage, labels locaux hors des boucles, `classify` mémoïsée, `eat` rapide, `-fwasm-exceptions` | `48ff6b4` | 367 ms |
| pliage : l'assembleur lit la source déroulée pliée | `be2fadb` | 311 ms |

Dans z80live : plus de 4 s → 950 ms après `48ff6b4`.

Outils : `make bench` (chronomètre, verrouille les empreintes du binaire, de
`-E` et de `-E --fold`), `FANTAMS_WASM=…/fantams.mjs` pour la colonne WASM,
`tests/fold_test` pour l'invariant du pliage.

## Où passe le temps maintenant

Sur `nested_if` : préprocesseur ≈ 245 ms, pliage ≈ 34 ms, assembleur ≈ 26 ms.
Profil du préprocesseur (gprof `-fno-inline`, `nested_if` ×8) :

| Poste | Part |
|---|---|
| `expr::eval` (conditions `if`, affectations, `noteAsmDefinition`) | ≈ 28 % |
| allocations et chaînes, diffuses (`_init` chez gprof : malloc, copies) | ≈ 23 % |
| `findMatching` (fermeture des `if` à chaque tour) | ≈ 11 % |
| `emit` (canonisation, sucre, sizeof) | ≈ 10 % |
| `noteAsmDefinition` | ≈ 6 % |

Il n'y a plus de point chaud isolé : chaque piste restante coûte un remaniement
pour 10 à 20 % du préprocesseur. Le chantier est suspendu, l'état actuel étant
jugé confortable. Les pistes sont dans `issues/`, sans ordre imposé.
