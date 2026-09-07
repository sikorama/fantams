#!/usr/bin/env bash
# build-wasm.sh — compile fantams (CLI bout-en-bout) vers WASM.
#
# Produit un module ES6 isomorphe (Node + navigateur) : factory
# `export default createFantams`, `callMain` + `FS` exposés, pas d'exécution
# auto : un hôte instancie le module, appelle `callMain` et lit ses artefacts
# dans `FS`. C'est le contrat le plus simple qui laisse l'hôte maître du moment
# et du nombre d'invocations.
#
# emcc n'étant pas requis en local, on passe par l'image officielle
# emscripten/emsdk sous podman (ou docker). Override : CONTAINER=docker.
#
#   ./build-wasm.sh           # ne recompile que si une source a bougé
#   ./build-wasm.sh --force   # recompile inconditionnellement
#
# Destinations (fantams ignore ce qu'en fait l'appelant) :
#   FANTAMS_OUT_DIR   où déposer fantams.mjs + fantams.wasm   (défaut : ./dist)
#   FANTAMS_PUB_DIR   copie supplémentaire, si l'hôte sert les artefacts
#                     depuis un autre dossier                 (défaut : aucune)
# Les chemins relatifs sont résolus depuis le dossier d'appel, pas depuis ici.
set -euo pipefail

FORCE=0
[ "${1:-}" = "--force" ] && FORCE=1

INVOKED_FROM="$PWD"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# `dirname/pwd` sur un chemin qui n'existe pas encore échouerait : on résout à la
# main, en n'exigeant que le parent.
resolve() {
  case "$1" in
    /*) printf '%s\n' "$1" ;;
    *)  printf '%s/%s\n' "$INVOKED_FROM" "$1" ;;
  esac
}

OUT_DIR="$(resolve "${FANTAMS_OUT_DIR:-$HERE/dist}")"
PUB_DIR="${FANTAMS_PUB_DIR:+$(resolve "$FANTAMS_PUB_DIR")}"

# Le podman écrit ses artefacts dans $HERE (volume monté) et le `mv` final les y
# reprend : le script doit donc s'y tenir, quel que soit l'endroit d'où on l'appelle.
cd "$HERE"

CONTAINER="${CONTAINER:-podman}"
IMAGE="${IMAGE:-docker.io/emscripten/emsdk:latest}"

CORE=(z80.cpp expr.cpp keywords.cpp parser.cpp pp.cpp asm.cpp link.cpp beautify.cpp sna.cpp sym.cpp asm_main.cpp)

# Note pile 8 Mo : le parseur récursif de fantams déborde la pile Emscripten
# par défaut (64 Ko) sur les grosses sources -> trap "table index out of bounds".
EMFLAGS=(
  -std=c++17 -O2
  # expr::eval s'appuie sur try/catch (throw EvalError). Sans -fexceptions,
  # Emscripten transforme tout throw en abort() -> "Aborted(undefined)".
  -fexceptions
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createFantams
  -sEXPORTED_RUNTIME_METHODS=callMain,FS
  -sINVOKE_RUN=0 -sEXIT_RUNTIME=0
  -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=8388608
  -sFORCE_FILESYSTEM=1
  -o fantams.mjs
)

# Test de fraîcheur. Le .wasm est un artefact compilé qui vit hors de l'arbre des
# sources qu'on édite tous les jours : rien dans git ne signale qu'il est en
# retard sur elles, et un .wasm périmé ne se manifeste que par des bugs déjà
# corrigés — le pire des symptômes, puisqu'il accuse le code plutôt que le build.
# On rend donc l'appel systématique bon marché, pour qu'il n'y ait jamais de
# raison de le sauter : il ne coûte le conteneur que si une source l'exige.
#
# La copie publiée compte comme une source de vérité : c'est elle que le
# navigateur charge, et un `git checkout` chez l'hôte peut la désynchroniser sans
# toucher aux .cpp.
is_stale() {
  [ "$FORCE" = 1 ] && { echo "--force"; return 0; }
  local w="$OUT_DIR/fantams.wasm"
  [ -f "$w" ] || { echo "$w absent"; return 0; }
  [ -f "$OUT_DIR/fantams.mjs" ] || { echo "$OUT_DIR/fantams.mjs absent"; return 0; }
  local f
  for f in "${CORE[@]}" "$HERE"/*.h; do
    [ -e "$f" ] || continue
    [ "$HERE/$(basename "$f")" -nt "$w" ] && { echo "$(basename "$f") plus récent que le .wasm"; return 0; }
  done
  if [ -n "$PUB_DIR" ] && [ -d "$PUB_DIR" ]; then
    cmp -s "$w" "$PUB_DIR/fantams.wasm" || { echo "$PUB_DIR désynchronisé"; return 0; }
  fi
  return 1
}

if ! reason="$(is_stale)"; then
  echo ">> WASM à jour, rien à recompiler (--force pour l'imposer)"
  exit 0
fi
echo ">> rebuild nécessaire : $reason"

echo ">> compilation WASM via $CONTAINER ($IMAGE)"
"$CONTAINER" run --rm -v "$HERE":/src:z -w /src "$IMAGE" \
  em++ "${EMFLAGS[@]}" "${CORE[@]}"

mkdir -p "$OUT_DIR"
mv -f fantams.mjs fantams.wasm "$OUT_DIR/"
echo ">> écrit : $OUT_DIR/fantams.mjs + fantams.wasm"
ls -l "$OUT_DIR/fantams.mjs" "$OUT_DIR/fantams.wasm"

# L'hôte peut servir les artefacts depuis un dossier public distinct (Vite, par
# exemple, ne sert que public/). On y recopie pour éviter la dérive.
if [ -n "$PUB_DIR" ]; then
  mkdir -p "$PUB_DIR"
  cp -f "$OUT_DIR/fantams.mjs" "$OUT_DIR/fantams.wasm" "$PUB_DIR/"
  echo ">> synchronisé -> $PUB_DIR/ (fantams.mjs, fantams.wasm)"
fi
