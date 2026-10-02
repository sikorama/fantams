#!/bin/sh
# bench.sh — le banc d'essai des sources déroulées longues (ADR 0034).
#
# Deux usages, une seule exécution :
#
#   1. CHRONOMÉTRER chaque source de tests/bench/, au meilleur de N essais, par
#      l'adaptateur natif et, si on le lui donne, par l'adaptateur WASM ;
#   2. VERROUILLER ce que ces sources produisent : l'empreinte du binaire, celle
#      de la source déroulée (-E) et celle de sa forme pliée (-E --fold). Une
#      optimisation qui change un octet de l'un d'eux est un bug, pas un gain.
#      Les empreintes vivent dans
#      tests/bench/expected.sha256.
#
# Le chronométrage n'est PAS dans « make test » : un temps dépend de la machine,
# et un test qui échoue sur une machine lente est un test qu'on désactive. Le
# verrou des empreintes, lui, échoue bruyamment (code 1).
#
#   tests/bench.sh                 chronomètre et vérifie
#   tests/bench.sh --record        réenregistre les empreintes (après un
#                                  changement VOULU de la sortie, à justifier)
#   FANTAMS_WASM=…/fantams.mjs     ajoute la colonne WASM
#   BENCH_RUNS=5                   nombre d'essais (3 par défaut)
set -e
# Un FANTAMS_WASM relatif se lit depuis le répertoire de l'APPELANT : le
# résoudre avant le cd, sans quoi il serait cherché sous la racine de fantams.
# Sous « make -C fantams bench », l'appelant est la racine : y écrire un chemin
# absolu, ou relatif à elle.
case "${FANTAMS_WASM:-}" in
    ''|/*) ;;
    *) FANTAMS_WASM=$(pwd)/$FANTAMS_WASM ;;
esac
cd "$(dirname "$0")/.."
ROOT=$(pwd)
FANTAMS=${FANTAMS:-./fantams}
RUNS=${BENCH_RUNS:-3}
DIR=tests/bench
SUMS=$DIR/expected.sha256
RECORD=0
[ "${1:-}" = "--record" ] && RECORD=1

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

WASM_MJS=${FANTAMS_WASM:-}
if [ -n "$WASM_MJS" ]; then
    [ -f "$WASM_MJS" ] || { echo "FANTAMS_WASM : $WASM_MJS introuvable" >&2; exit 2; }
    command -v node >/dev/null 2>&1 || { echo "FANTAMS_WASM donné mais node absent" >&2; exit 2; }
fi

now() { date +%s%N; }
ms() { echo $(( ($2 - $1) / 1000000 )); }

# Meilleur de $RUNS essais de la commande donnée, en millisecondes.
best() {
    b=
    k=0
    while [ $k -lt "$RUNS" ]; do
        t0=$(now); "$@" >/dev/null 2>&1; t1=$(now)
        t=$(ms "$t0" "$t1")
        { [ -z "$b" ] || [ "$t" -lt "$b" ]; } && b=$t
        k=$((k + 1))
    done
    echo "$b"
}

wasm() { node "$ROOT/tests/wasm_cli.mjs" "$WASM_MJS" "$@"; }

: > "$TMP/sums"
if [ -n "$WASM_MJS" ]; then
    # Le coût fixe de node + instanciation du module, à soustraire à l'œil.
    printf '    nop\n' > "$TMP/empty.asm"
    base=$(best wasm --in "$TMP/empty.asm:/in.asm" -- /in.asm -o /out.bin)
    printf '%-14s %10s %10s\n' source natif WASM
    printf '%-14s %10s %8sms\n' "(démarrage)" - "$base"
else
    printf '%-14s %10s\n' source natif
fi

for src in "$DIR"/*.asm; do
    name=$(basename "$src" .asm)
    "$FANTAMS" "$src" -o "$TMP/$name.bin" >/dev/null 2>&1 ||
        { echo "ECHEC : $src ne s'assemble pas"; "$FANTAMS" "$src" -o "$TMP/$name.bin"; exit 1; }
    "$FANTAMS" -E "$src" -o "$TMP/$name.E.asm" >/dev/null 2>&1 ||
        { echo "ECHEC : -E refuse $src"; exit 1; }
    "$FANTAMS" -E --fold "$src" -o "$TMP/$name.fold.asm" >/dev/null 2>&1 ||
        { echo "ECHEC : -E --fold refuse $src"; exit 1; }
    (cd "$TMP" && sha256sum "$name.bin" "$name.E.asm" "$name.fold.asm") >> "$TMP/sums"

    nat=$(best "$FANTAMS" "$src" -o "$TMP/t.bin")
    if [ -n "$WASM_MJS" ]; then
        w=$(best wasm --in "$ROOT/$src:/in.asm" -- /in.asm -o /out.bin)
        printf '%-14s %8sms %8sms\n' "$name" "$nat" "$w"
    else
        printf '%-14s %8sms\n' "$name" "$nat"
    fi
done

if [ $RECORD = 1 ]; then
    cp "$TMP/sums" "$SUMS"
    echo "empreintes enregistrées dans $SUMS"
    exit 0
fi
[ -f "$SUMS" ] || { echo "ECHEC : $SUMS absent — lancer « tests/bench.sh --record » sur une version de référence"; exit 1; }
if ! diff "$SUMS" "$TMP/sums" >"$TMP/diff"; then
    echo "ECHEC : la sortie a changé (binaire ou source déroulée) :"
    cat "$TMP/diff"
    exit 1
fi
echo "empreintes : identiques"
