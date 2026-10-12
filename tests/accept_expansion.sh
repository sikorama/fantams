#!/bin/sh
# accept_expansion.sh — une expansion est un espace de noms (ADR 0038).
#
# Les règles de nommage se testent au préprocesseur (tests/pp_test.cpp). Ce
# script vérifie ce qu'on ne voit qu'en sortant du processus réel : l'exemple
# qui les illustre assemble, le fichier de symboles montre les noms par
# expansion, et les deux erreurs voulues sortent en code non nul avec leur
# message.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

fail() { echo "ECHEC : $1" >&2; exit 1; }

# 1. L'exemple assemble, et --sym montre un nom par expansion.
$FANTAMS examples/demo.asm --sym="$TMP/demo.sym" -o "$TMP/demo.bin" >/dev/null 2>&1 \
    || fail "examples/demo.asm n'assemble pas"
grep -q '^WAIT__1\.wloop,' "$TMP/demo.sym" || fail "--sym ne montre pas WAIT__1.wloop"
grep -q '^WAIT__2\.wloop,' "$TMP/demo.sym" || fail "--sym ne montre pas WAIT__2.wloop"
if grep -q '^wloop,' "$TMP/demo.sym"; then fail "wloop ne devrait pas être global"; fi

# 2. Un module et son exemple : le nom qualifié remplace @@export.
$FANTAMS examples/struct_module.asm -o "$TMP/sm.bin" >/dev/null 2>&1 \
    || fail "examples/struct_module.asm n'assemble pas"

# 3. Un label construit par argument est global ; deux appels de même valeur
#    sont une vraie collision.
cat > "$TMP/arg.asm" <<'EOS'
        org 0x4000
MACRO FN(name)
mon_{name}: nop
ENDM
        FN(a)
        FN(b)
        jp mon_a
        jp mon_b
EOS
$FANTAMS "$TMP/arg.asm" --sym="$TMP/arg.sym" -o "$TMP/arg.bin" >/dev/null 2>&1 \
    || fail "deux appels de valeurs différentes devraient assembler"
grep -q '^mon_a,' "$TMP/arg.sym" || fail "mon_a absent de --sym"
grep -q '^mon_b,' "$TMP/arg.sym" || fail "mon_b absent de --sym"

cat > "$TMP/dup.asm" <<'EOS'
        org 0x4000
MACRO FN(name)
mon_{name}: nop
ENDM
        FN(a)
        FN(a)
EOS
if $FANTAMS "$TMP/dup.asm" -o "$TMP/dup.bin" >"$TMP/dup.out" 2>&1; then
    fail "deux appels qui construisent le même nom devraient échouer"
fi
grep -q "duplicate symbol: 'mon_a'" "$TMP/dup.out" || fail "message de collision absent"

# 4. Un label @ est global et ne permet qu'une expansion.
cat > "$TMP/once.asm" <<'EOS'
        org 0x4000
MACRO M
@entry: nop
ENDM
        M
        jp entry
EOS
$FANTAMS "$TMP/once.asm" -o "$TMP/once.bin" >/dev/null 2>&1 \
    || fail "une macro à label @ développée une fois devrait assembler"

printf '        org 0x4000\nMACRO M\n@entry: nop\nENDM\n        M\n        M\n' > "$TMP/twice.asm"
if $FANTAMS "$TMP/twice.asm" -o "$TMP/twice.bin" >"$TMP/twice.out" 2>&1; then
    fail "une macro à label @ développée deux fois devrait échouer"
fi
grep -q "marks it with '@'" "$TMP/twice.out" || fail "message du label @ absent"

echo "acceptation : une expansion est un espace de noms — exemples, --sym et erreurs voulues"
