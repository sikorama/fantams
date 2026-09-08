# Piloter AMSpiriT-Lite depuis fantams — relevé de l'API et du harnais existant

Objet : établir ce que l'API HTTP d'AMSpiriT-Lite permet de faire pour (a) exécuter
un livrable de fantams et vérifier son effet, et (b) valider les conteneurs que
`docs/spec-chaine-outils.md` prévoit d'ajouter. Ce document est un relevé de
sources primaires, pas une conception : il alimente le grilling sur la couture
entre fantams et le harnais de test.

Date du relevé : 2026-09-08. L'émulateur tournait en local sur
`127.0.0.1:8765` et son `/api/doc` a été interrogé directement.

---

## 0. Sources

| Réf | Source | Autorité |
|-----|--------|----------|
| **[DOC]** | `GET /api/doc` et `GET /api/doc/<nom>` de l'instance vivante (127.0.0.1:8765) | **Contrat machine, projection du *route record*** — la source de vérité |
| **[MD]** | `/var/home/siko/Code/Amspirit/amspirit-lite/src/doc/web_server_api.md` (1882 l.) | Companion narratif de [DOC] : nuances de comportement et pièges. Sa cohérence avec le route record est vérifiée par `scripts/check-api-contract.py`, exécuté par `make test` |
| **[SMOKE]** | `/var/home/siko/Code/Amspirit/amspirit-lite/scripts/smoke-headless.sh` (73 l.) | Test d'intégration réel du frontend headless — **le patron de pilotage de référence** |
| **[Z80L]** | `/var/home/siko/Code/z80live/z80next/app/src/lib/amspirit.js` (39 l.) | Le client actuel de z80live |

Le route record lui-même est `amspirit-helpers/src/web_handle.cpp`, de forme
déclarée dans `web_doc.h`. Un endpoint ajouté sans documentation fait échouer
la suite de tests d'amspirit-lite : **le contrat est vérifié, pas promis**.

---

## 1. Le transport

Serveur HTTP minimal démarré automatiquement par les frontends. **Mono-thread,
une requête à la fois, pas de keep-alive.** N'écoute que sur la boucle locale,
jamais exposé sur le réseau. Port par défaut `8765`, réglable
(`WebServerOpts.port`, option `--web-port`). **Si le port est occupé, le serveur
reste simplement désactivé — pas d'erreur fatale**, donc un test qui suppose
l'API disponible doit sonder `/api/ping` et non présumer.

Conséquence directe pour un harnais : *une* instance sert *une* requête à la
fois. Une suite de tests parallèle (`ctest -j`) doit soit sérialiser les tests
qui touchent l'émulateur, soit donner un `--web-port` distinct à une instance
par test.

## 2. Le frontend headless existe déjà

`src/amspirit-headless` (meson) produit `amspirit-lite-headless` : ni écran ni
périphérique audio requis, il tourne en CI. [SMOKE] en donne le cycle de vie
complet, qui est exactement celui dont fantams a besoin :

```sh
"$BIN" -C "$WORK/cfg" -R "$ROMS" --web-port "$PORT" &   # config jetable
# attente du boot : sonder /api/ping jusqu'à réponse (le serveur démarre après
# le chargement des ROM) — jusqu'à 50 × 0,2 s
# vérifier que l'émulation avance réellement : emu.frames croît (~50/s)
# … pilotage …
curl -X POST "$BASE/api/quit"                            # arrêt ordonné
wait "$PID"; [ "$RC" -eq 0 ]                             # doit sortir 0
```

Trois invariants de ce patron valent d'être repris tels quels : la **config
jetable** (`-C` sur un `mktemp -d`, le test ne touche ni ne dépend d'une config
utilisateur), l'**attente par sondage** plutôt qu'un `sleep` fixe, et l'arrêt
par `/api/quit` avec vérification du code de sortie.

## 3. Charger un livrable — `POST /api/media`

Un seul endpoint charge **tous** les conteneurs, corps = octets bruts du
fichier, réponse `{ok}`. Le format est **autodétecté par octets magiques**, comme
partout dans l'application :

| Conteneur | Détection | Effet |
|-----------|-----------|-------|
| `.sna` | contenu (magie `MV - SNA`) | restaure l'état machine complet (registres + RAM) |
| `.dsk` / `.hfe` / `.ipf` | contenu | insère dans le lecteur `drive` (`0`=A, `1`=B) |
| `.cpr` | contenu (magie `RIFF…AMS`) | chargé comme cartouche standard |
| `.cro` | contenu (magie `RIFF…CRO `) | **déclenche un hard reset** |
| `.bin` / `.amsdos` | **l'extension du paramètre `name`, pas le contenu** | un binaire à en-tête AMSDOS se charge directement ; un binaire brut sans en-tête exige une adresse explicite : `name=jeu@4000.bin`, l'entrée en option : `name=jeu@4000@4000.bin`. Quatre chiffres hexa, 16 bits à plat — **pas de préfixe `Bnn:`** (le `:` est illégal dans un nom de fichier Windows) |
| `.cdt` | — | **limitation connue** : pas de signature de contenu et pas routé par extension non plus, tombe dans le chemin binaire brut et ne se charge pas comme cassette |

**C'est le résultat le plus important de ce relevé** : les conteneurs
qu'AMSpiriT sait charger — SNA, DSK, CPR, CRO, binaire à en-tête AMSDOS — sont
exactement ceux que fantams prévoit d'émettre. L'émulateur peut donc servir
d'**oracle de conformité** pour l'étage conteneurs : émettre, poster, relire la
RAM, comparer à l'attendu. Sans lui, la seule vérification possible d'un `.dsk`
est une relecture par notre propre code, qui ne prouve rien.

À noter pour le grilling : la forme `nom@ADDR@ENTRY.bin` est **la convention
d'AMSpiriT, pas la nôtre**, et elle est plate 16 bits là où l'ADR 0005 nous fait
écrire `Bn:adresse` (cf. le piège du §5). Elle ne concerne que le binaire *sans*
en-tête ; un binaire à en-tête AMSDOS porte son adresse de chargement dans ses
octets et n'en a pas besoin — ce qui fait de l'en-tête AMSDOS le conteneur le
plus simple à valider par cette voie.

## 4. Lire l'état — ce sur quoi un test peut assertir

| Endpoint | Ce qu'il donne |
|----------|----------------|
| `GET /api/ram?addr&len&bank&view` | fenêtre de RAM en hexa. `bank` : `0-3` = 64K de base, `4+` = extension (B00–B103) ; une `addr` au-delà de `0x3FFF` se propage dans la banque. `view` : `raw` (la banque elle-même), `cpu` (la vue mappée du Z80, overlays ROM appliqués), `fw` (vue firmware) → `{addr,len,[view,]hex}` |
| `GET /api/z80` | tous les registres, y compris secondaires, `I`, `R`, `IFF1/2`, `IM` |
| `GET /api/memmap` | mapping ROM/RAM par région de 16K : `{regions:[{base,name,rom,rom_bank\|ram_bank,ext}],rmr,ram_mode,ram_page}` |
| `GET /api/state` | agrégat z80/ga/crtc/psg/fdc/emu en un appel |
| `GET /api/screenshot` | PNG de la trame courante (+ en-têtes `X-Beam-*`, `X-Crop-*`) |
| `GET /api/events` | SSE, un `data: {…}` (forme de `/api/state`) **par trame émulée** |

Le triplet `bank` / `view` de `/api/ram` recoupe directement notre modèle
mémoire (ADR 0006, espaces d'adressage) et la notation préfixée par la banque
(ADR 0005) : `view=raw` est ce qu'un test doit lire pour vérifier des octets
posés dans une banque nommée, `view=cpu` ce qu'il doit lire pour vérifier ce
que le Z80 voit réellement.

## 5. S'arrêter à un point connu

| Endpoint | Rôle |
|----------|------|
| `POST /api/z80_bp` | **remplace** l'ensemble des points d'arrêt PC. Corps texte : liste séparée par virgules, adresse CPU décimale ou `0x`hexa, **ou physique `Bnn:hhhh`** (`nn` = banque 16K, hexa). Corps vide = tout effacer |
| `POST /api/raster_bp` | point d'arrêt sur position du faisceau (`x`, `y`, `enable`) |
| `POST /api/step` | exactement une instruction Z80, puis re-pause |
| `POST /api/config` | `paused`, `do_soft_reset`, `do_hard_reset`, et le modèle : `cpc_model`, `crtc_type`, `ram_kb`, `rom_lang` |
| `POST /api/ram` (`exec`, `entry`) | écriture RAM et/ou redirection du PC, appliquées à l'itération suivante de la boucle principale → `{ok,seq}` |
| `POST /api/exec` | raccourci : redirection du PC seule |

### Piège : `Bnn:hhhh` est un faux ami de notre `Bn:adresse`

`POST /api/z80_bp` accepte une notation physique de **graphie identique** à celle
de l'ADR 0005, et de **sémantique différente**. C'est vérifié des deux côtés, pas
supposé :

| | fantams (ADR 0005) | AMSpiriT ([MD] l. 1534) |
|---|---|---|
| forme | `Bn:adresse`, ex. `org b4:0x4000` | `Bnn:hhhh`, ex. `B00:4100` |
| numéro de banque | décimal | **hexa** (`B00`–`B103`) |
| ce que dénote le nombre qui suit | l'**adresse logique** — celle que prennent les labels | un **offset dans la banque** |
| au-delà de `0x3FFF` | **masqué** : offset = `adresse & 0x3FFF`, on reste dans la banque | **report** : « an offset above `0x3FFF` carries into the bank », donc `B00:4100` est le même octet que `B01:0100` |

Donc `b4:0x4000` (fantams) et `B04:4000` (AMSpiriT) **ne désignent pas le même
octet** : le premier est la banque 4 à l'offset 0, le second la banque 5 à
l'offset 0. Le numérotage des banques, lui, concorde : celui d'AMSpiriT est
« per 16 Ko » et `regions[].ram_bank` de `/api/memmap` emploie la même échelle
(`0-3` = 64K de base, `4+` = extension), comme la nôtre.

Un harnais ne peut donc pas passer une adresse de notre source telle quelle à
`/api/z80_bp` : il doit traduire. La traduction est mécanique — banque en hexa,
et l'offset `adresse & 0x3FFF` que notre ADR définit déjà — mais elle doit être
écrite une fois, à un seul endroit, et nommée pour ce qu'elle est.

`POST /api/config` permet de **choisir la machine** par test : un test de profil
464 et un test de profil 6128 peuvent viser la même instance.

## 6. Aussi disponible, pas encore nécessaire

`POST /api/script` et `POST /api/eval` exécutent du CSL ou du Lua sur un état
persistant, en bac à sable (pas de `io`/`package`/`debug`/`require`, `os` réduit
à `time`/`clock`/`date`, fichiers via une table `fs.*` en prison). `/api/eval`
est asynchrone : le POST rend un `seq` à sonder en GET, `done=false` tant que le
chunk tourne, `refused=true` si le moteur était occupé. Une assertion complexe
pourrait un jour vivre là plutôt que dans une cascade de `curl`.

`tools/mcp-emulator/server.py` expose par ailleurs des endpoints en outils MCP,
et `check-api-contract.py` vérifie que ce wrapper n'appelle rien d'indocumenté.

## 7. Ce que z80live fait aujourd'hui

[Z80L] fait 39 lignes et exactement deux choses : `ping(base)` (sonde avec
`AbortController`, timeout 800 ms, appelée en polling depuis l'UI) et
`injectSna(base, bytes)` (un seul `POST /api/media`, format autodétecté).

Son commentaire porte un enseignement à ne pas perdre :

> Remplace l'ancienne approche pause + écriture RAM par blocs + redirection PC,
> qui butait sur le pause côté AMSpiriT ne tenant pas de façon fiable.

Autrement dit : **ne pas reconstruire l'injection à coups de `POST /api/ram` +
`/api/exec`**. Ce chemin a déjà été essayé et abandonné ; `/api/media` avec un
`.sna` est la voie éprouvée. Le reste du fichier est du `fetch` de navigateur en
ESM : il n'y a pas là de logique substantielle à porter.

---

## 8. Questions ouvertes — pour le grilling, pas pour ce relevé

1. **Le harnais est-il en bash ou ailleurs ?** Les tests d'acceptation de
   fantams sont déjà des scripts bash câblés par `add_test` (`accept_profile`,
   `accept_banked`, `accept_separate`, `no_machine_names`), et [SMOKE] est du
   bash. Rien à porter depuis le JS de z80live.
2. **Une instance par test, ou une partagée ?** Le serveur est mono-thread et le
   port se règle ; l'arbitrage est entre coût de démarrage (chargement des ROM)
   et isolation.
3. **L'émulateur est-il une dépendance obligatoire de `ctest`, ou une suite
   optionnelle** sautée quand le binaire headless est absent ? Il vient d'un
   autre dépôt et exige un jeu de ROM (`-R`).
4. **Où est la ROM ?** [SMOKE] passe `-R src/ROMs` depuis le dépôt
   d'amspirit-lite. Un harnais fantams doit trouver ce répertoire.
5. **Qui porte la traduction `Bn:adresse` → `Bnn:hhhh` ?** La divergence est
   établie (§5), pas la place du convertisseur : dans le harnais, ou exposé par
   fantams lui-même comme une graphie de sortie ?
6. **Que devient z80live ?** Consommateur d'un harnais partagé, ou simple
   client indépendant qui continue sa vie de son côté ?
