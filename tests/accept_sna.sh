#!/bin/sh
# accept_sna.sh — le snapshot et le binaire brut, par la CLI reelle.
#
# sna_test verifie l'en-tete et la pose sur une base a partir d'octets
# fabriques. Ce qui ne se verifie qu'ici, c'est le branchement :
#
#   1. un .sna de 64 Ko, de 128 Ko des qu'une banque etendue est ecrite ;
#   2. --base : une v2 est posee dessous, le reste est refuse, sans sortie ;
#   3. au-dela de la banque 7, ni .sna ni binaire brut ne sont ecrits ;
#   4. le binaire brut : l'intervalle contigu, et l'avertissement quand des
#      banques etendues sont perdues.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

fail() { echo "ECHEC : $1" >&2; [ -f "$TMP/log" ] && cat "$TMP/log" >&2; exit 1; }
size() { wc -c < "$1" | tr -d ' '; }

printf '        org 0x8000\n        run start\nstart:  ld a,1\n        ret\n' > "$TMP/p.asm"
printf '        section rom, "ro"\n        db 1\n' > "$TMP/r.asm"
printf 'TARGET cpc6128\nMEMORY_MAP { CONFIG rom_upper.on<7> { w3 { SECTION rom } } }\n' > "$TMP/r.ld"
printf '        section ext, "rw"\n        db 0x55\n' > "$TMP/e.asm"
printf 'TARGET cpc6128\nMEMORY_MAP { CONFIG ram.ext_w1<1> { w1 { SECTION ext } } }\n' > "$TMP/e.ld"

# 1. 64 Ko, puis 128 Ko.
"$FANTAMS" "$TMP/p.asm" -o "$TMP/p.sna" >"$TMP/log" 2>&1 || fail ".sna refuse"
[ "$(size "$TMP/p.sna")" = 65792 ] || fail "p.sna fait $(size "$TMP/p.sna") octets, attendu 65792"
grep -q "p.sna: snapshot (65792 bytes), PC=0x8000" "$TMP/log" || fail "bilan du .sna inattendu"
pc=$(od -An -tx1 -j 35 -N 2 "$TMP/p.sna" | tr -d ' \n')
[ "$pc" = 0080 ] || fail "PC de l'en-tete = $pc, attendu 0080"
"$FANTAMS" "$TMP/e.asm" -T "$TMP/e.ld" -o "$TMP/e.sna" >"$TMP/log" 2>&1 || fail ".sna etendu refuse"
[ "$(size "$TMP/e.sna")" = 131328 ] || fail "e.sna fait $(size "$TMP/e.sna") octets, attendu 131328"
echo "acceptation : un .sna de 64 Ko, de 128 Ko avec une banque etendue"

# 2. --base.
head -c 65792 "$TMP/p.sna" > "$TMP/v2.sna"
printf '\002' | dd of="$TMP/v2.sna" bs=1 seek=16 conv=notrunc 2>/dev/null
"$FANTAMS" "$TMP/p.asm" -o "$TMP/b.sna" --base "$TMP/v2.sna" >"$TMP/log" 2>&1 || fail "--base v2 refusee"
grep -q "(CPCType 2)" "$TMP/log" || fail "le bilan ne nomme pas la base"
[ "$(size "$TMP/b.sna")" = 65792 ] || fail "b.sna fait $(size "$TMP/b.sna") octets"

refuse() {
    what=$1; out=$2; shift 2
    if "$FANTAMS" "$@" >"$TMP/err" 2>&1; then fail "$what devrait etre refuse"; fi
    grep -q -- "$REASON" "$TMP/err" || { cat "$TMP/err"; fail "$what : le message ne nomme pas '$REASON'"; }
    [ ! -e "$out" ] || fail "$what : $out a ete ecrit"
}
REASON="reenregistre-la en version 2" refuse "--base v3" "$TMP/x1.sna" "$TMP/p.asm" -o "$TMP/x1.sna" --base "$TMP/p.sna"
echo junk > "$TMP/bad.sna"
REASON="signature" refuse "--base illisible" "$TMP/x2.sna" "$TMP/p.asm" -o "$TMP/x2.sna" --base "$TMP/bad.sna"
REASON="base introuvable" refuse "--base absente" "$TMP/x3.sna" "$TMP/p.asm" -o "$TMP/x3.sna" --base "$TMP/nope.sna"
echo "acceptation : --base v2 posee dessous, le reste refuse sans sortie"

# 3. Au-dela de la banque 7.
REASON="bank(s) 9 written, beyond bank 7" refuse ".sna en banque 9" "$TMP/r.sna" "$TMP/r.asm" -T "$TMP/r.ld" -o "$TMP/r.sna"
REASON="bank(s) 9 written, beyond bank 7" refuse ".bin en banque 9" "$TMP/r.bin" "$TMP/r.asm" -T "$TMP/r.ld" -o "$TMP/r.bin"
echo "acceptation : au-dela de la banque 7, rien n'est ecrit"

# 4. Le binaire brut.
"$FANTAMS" "$TMP/p.asm" -o "$TMP/p.bin" >"$TMP/log" 2>&1 || fail ".bin refuse"
[ "$(od -An -tx1 "$TMP/p.bin" | tr -d ' \n')" = 3e01c9 ] || fail "p.bin ne porte pas 3e01c9"
grep -q "p.bin: 3 bytes @ 0x8000" "$TMP/log" || fail "bilan du .bin inattendu"
"$FANTAMS" "$TMP/e.asm" -T "$TMP/e.ld" -o "$TMP/e.bin" >"$TMP/log" 2>&1 || fail ".bin etendu refuse"
grep -q "warning: banks beyond the base 64K were written" "$TMP/log" || fail "pas d'avertissement pour les banques perdues"
echo "acceptation : le binaire brut, et l'avertissement des banques perdues"
