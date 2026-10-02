#!/bin/sh
# accept_cro.sh — l'export conteneur de ROMs, par la CLI reelle (ADR 0033).
#
# cro_test verifie la traduction et la fusion sur des images fabriquees a la
# main. Ce qui ne se verifie qu'ici, c'est le branchement :
#
#   1. « -o x.cro --cro-rom n », une invocation par ROM, AJOUTE chaque ROM au
#      fichier deja sur disque — deux invocations, deux ROMs dans un groupe ;
#   2. les combinaisons qui n'ont pas de sens sont refusees avant l'assemblage.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for n in 7 15; do
    cat > "$TMP/rom$n.asm" <<EOF
        section rom, "ro"
        db 1, $n
EOF
    cat > "$TMP/rom$n.ld" <<EOF
TARGET cpc6128
MEMORY_MAP { CONFIG rom_upper.on<$n> { w3 { SECTION rom } } }
EOF
done

# 1. Deux ROMs hautes, deux invocations, un fichier.
OUT="$TMP/sys.cro"
for n in 7 15; do
    "$FANTAMS" "$TMP/rom$n.asm" -T "$TMP/rom$n.ld" -o "$OUT" --cro-rom $n >"$TMP/log" 2>&1 ||
        { echo "ECHEC : --cro-rom $n refuse"; cat "$TMP/log"; exit 1; }
done

# 12 (RIFF) + 8 (GRRO) + 12 (GNUM) + 12 (GLBL « sys », padde) + 12 (GMSK)
# + 2 x 16448 (ROM).
size=$(wc -c < "$OUT" | tr -d ' ')
[ "$size" = 32952 ] || { echo "ECHEC : $OUT fait $size octets, attendu 32952 (deux ROMs)"; exit 1; }

at() { od -An -c -j "$1" -N "$2" "$OUT" | tr -d ' \n'; }
hex() { od -An -tx1 -j "$1" -N "$2" "$OUT" | tr -d ' \n'; }
[ "$(at 8 4)" = "CRO" ] || { echo "ECHEC : forme RIFF $(at 8 4), attendu 'CRO '"; exit 1; }
[ "$(at 40 3)" = "sys" ] || { echo "ECHEC : GLBL $(at 40 3), attendu le nom du fichier 'sys'"; exit 1; }
[ "$(at 72 4)" = "hi07" ] || { echo "ECHEC : premiere ROM $(at 72 4), attendu hi07"; exit 1; }
[ "$(at 16520 4)" = "hi15" ] || { echo "ECHEC : seconde ROM $(at 16520 4), attendu hi15"; exit 1; }
[ "$(hex 16544 4)" = "0f000000" ] || { echo "ECHEC : RLOG de hi15 = $(hex 16544 4), attendu 15"; exit 1; }
[ "$(hex 16568 2)" = "010f" ] || { echo "ECHEC : RDT de hi15 commence par $(hex 16568 2), attendu 010f"; exit 1; }
echo "acceptation : deux invocations --cro-rom ajoutent deux ROMs au meme .cro"

# 2. Les refus, avant tout assemblage.
refuse() {
    what=$1; shift
    if "$FANTAMS" "$@" >"$TMP/err" 2>&1; then
        echo "ECHEC : $what devrait etre refuse"; exit 1
    fi
    grep -q -- "$REASON" "$TMP/err" || { echo "ECHEC : $what : le message ne nomme pas '$REASON'"; cat "$TMP/err"; exit 1; }
}
REASON=--cro-rom refuse "--cro-rom sans .cro" "$TMP/rom7.asm" -T "$TMP/rom7.ld" -o "$TMP/x.bin" --cro-rom 7
REASON=--cro-group refuse "--cro-group sans .cro" "$TMP/rom7.asm" -T "$TMP/rom7.ld" -o "$TMP/x.bin" --cro-group 1
REASON=--base refuse ".cro avec --base" "$TMP/rom7.asm" -T "$TMP/rom7.ld" -o "$TMP/x.cro" --cro-rom 7 --base "$TMP/b.sna"
REASON=-E refuse ".cro avec -E" "$TMP/rom7.asm" -o "$TMP/x.cro" -E
echo "acceptation : les combinaisons sans objet sont refusees"
