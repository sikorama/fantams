# cpr20 — 21 banques ROM sur cartouche CPC+, sans compression

Le test simple qui devait prouver que la chaine CPR marche avant d'y ajouter
la compression (etape 2) : une ROM de demarrage (banque physique 0) qui
affiche, l'une apres l'autre, 20 banques de 16 Ko de donnees (banques 1..20),
chacune un motif different et trivial a reconnaitre.

## Ce que ca prouve

- L'axe de profil `cart_rom_hi` (ajoute a `cpcplus` dans `profiles.cpp`) :
  les banques physiques 8..31 d'une cartouche ne sont adressables qu'en ROM
  HAUTE (`docs/recherche/cpc-gate-array-rmr.md` §D.1) — un mecanisme distinct
  de `cart_rom` (RMR2, banques 0..7 seulement), mais qui alimente le MEME
  conteneur CPR parce qu'il partage la meme banque de profil (`crom<n>`,
  STORE 16).
- La CLI sait maintenant PRODUIRE un `.cpr` : `-o x.cpr --cpr-bank <n>`
  (voir `asm_main.cpp`, et `cpr::extractOne`/`cpr::merge` dans `cpr.cpp`).
  Chaque banque physique est liee SEPAREMENT (`cpr.h` l'imposait deja) et
  AJOUTEE au conteneur — une invocation par banque, comme `-o x.fo` pour la
  compilation separee. `build.sh` en fait 21, une par banque.
- Que la sequence materielle (deux ecritures, `GA_PORT` puis `&DF00`, cf.
  `__port_`/`__port2_`/`__romnum_` du §12.3) fonctionne reellement sur un
  CPC+ (verifie a l'emulateur, pas seulement aux tests unitaires).

## Ce que ca NE couvre PAS encore

- **L'app (UI) ne sait toujours pas produire un `.cpr`** : l'option est
  desactivee ("coming soon"). Seule la CLI `fantams` le fait, pour l'instant.
- Aucune compression (etape 2 du projet).
- Le CRTC et la palette sont initialises A LA MAIN dans `boot.asm` : sur une
  vraie cartouche Plus, RIEN ne tourne avant ce code (RMR2 y mappe la ROM 0
  a la place du firmware des le reset, §D.1) — pas de firmware pour le
  faire a notre place.

## Fichiers

| fichier | role |
|---|---|
| `boot.asm` / `boot.ld` | banque physique 0. `di`, CRTC (les valeurs standard données), palette (4 crayons du mode 1), puis boucle : bascule la ROM haute sur la banque physique N (2 ecritures, cf. plus haut), `ldir #C000->#C000` (16 Ko) — l'astuce classique : la ROM en lecture, l'ecriture retombe sur la RAM ecran sous-jacente — pause grossiere, banque suivante. |
| `gen_banks.sh` | engendre `bank01.asm`/`.ld`..`bank20.asm`/`.ld` : chacun une `SECTION cart` de 16 Ko (`REPEAT` d'un motif `(n, 255-n)` different par banque) et le script qui la place en `cart_rom_hi.on<n>`. Non commis, reproductible. |
| `build.sh` | assemble et lie les 21 banques, une invocation `fantams ... -o cpr20.cpr --cpr-bank n` a la fois. |

## Utilisation

```
./build.sh                 # ecrit cpr20.cpr ici
curl -X POST --data-binary @cpr20.cpr "http://localhost:8765/api/media?name=cpr20.cpr"
curl -X POST -d '{"do_hard_reset":true}' -H 'Content-Type: application/json' http://localhost:8765/api/config
```

Verifie a l'emulateur CPC+ (via son API `/api/*`) le 2026-09-10 : le CRTC
reprend exactement les registres donnes, les 20 motifs s'affichent l'un
apres l'autre en bandes verticales de mode 1, et une capture prise pendant
un `ldir` montre meme la dechirure (moitie ancien motif, moitie nouveau) —
la preuve que la copie ecrase bien l'ecran en direct plutot que de dessiner
une image deja prete. Le fichier produit par `build.sh` (via la CLI) est
BYTE POUR BYTE identique a celui du harnais jetable qui avait servi a la
premiere verification.
