#!/bin/sh
# accept_cpr.sh — l'export cartouche CPR, par la CLI reelle.
#
# Ce qui ne se verifie qu'ici, c'est le branchement :
#
#   1. « -o x.cpr --cpr-bank n », une invocation par banque, AJOUTE chaque
#      banque au fichier deja sur disque, et le bilan le dit ;
#   2. les refus : banque absente ou hors de 0..31, combinaisons sans objet,
#      conteneur existant mal forme — laisse intact.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for n in 0 3; do
    cat > "$TMP/b$n.asm" <<EOF
        section cart, "ro"
        db 1, $n
EOF
    cat > "$TMP/b$n.ld" <<EOF
TARGET cpcplus
MEMORY_MAP { CONFIG cart_rom.w0<$n> { w0 { SECTION cart } } }
EOF
done

# 1. Deux banques, deux invocations, un fichier.
OUT="$TMP/cart.cpr"
for n in 0 3; do
    "$FANTAMS" "$TMP/b$n.asm" -T "$TMP/b$n.ld" -o "$OUT" --cpr-bank $n >"$TMP/log" 2>&1 ||
        { echo "ECHEC : --cpr-bank $n refuse"; cat "$TMP/log"; exit 1; }
done
grep -q "cart.cpr: banque physique 3 (16384 octets), 2 banque(s) au total" "$TMP/log" ||
    { echo "ECHEC : bilan inattendu"; cat "$TMP/log"; exit 1; }

# 12 (RIFF + AMS!) + 2 x (8 + 16384).
size=$(wc -c < "$OUT" | tr -d ' ')
[ "$size" = 32796 ] || { echo "ECHEC : $OUT fait $size octets, attendu 32796 (deux banques)"; exit 1; }

at() { od -An -c -j "$1" -N "$2" "$OUT" | tr -d ' \n'; }
hex() { od -An -tx1 -j "$1" -N "$2" "$OUT" | tr -d ' \n'; }
[ "$(at 8 4)" = "AMS!" ] || { echo "ECHEC : forme RIFF $(at 8 4), attendu AMS!"; exit 1; }
[ "$(at 12 4)" = "cb00" ] || { echo "ECHEC : premier chunk $(at 12 4), attendu cb00"; exit 1; }
[ "$(hex 20 2)" = "0100" ] || { echo "ECHEC : cb00 commence par $(hex 20 2), attendu 0100"; exit 1; }
[ "$(at 16404 4)" = "cb03" ] || { echo "ECHEC : second chunk $(at 16404 4), attendu cb03"; exit 1; }
[ "$(hex 16412 2)" = "0103" ] || { echo "ECHEC : cb03 commence par $(hex 16412 2), attendu 0103"; exit 1; }

# Reconstruire une banque la remplace, sans la dupliquer.
"$FANTAMS" "$TMP/b0.asm" -T "$TMP/b0.ld" -o "$OUT" --cpr-bank 0 >"$TMP/log" 2>&1 ||
    { echo "ECHEC : reconstruction de la banque 0 refusee"; cat "$TMP/log"; exit 1; }
size=$(wc -c < "$OUT" | tr -d ' ')
[ "$size" = 32796 ] || { echo "ECHEC : apres remplacement, $size octets, attendu 32796"; exit 1; }
echo "acceptation : deux invocations --cpr-bank ajoutent deux banques au meme .cpr"

# 2. Les refus.
refuse() {
    what=$1; shift
    if "$FANTAMS" "$@" >"$TMP/err" 2>&1; then
        echo "ECHEC : $what devrait etre refuse"; exit 1
    fi
    grep -q -- "$REASON" "$TMP/err" || { echo "ECHEC : $what : le message ne nomme pas '$REASON'"; cat "$TMP/err"; exit 1; }
}
REASON="demande --cpr-bank" refuse ".cpr sans --cpr-bank" "$TMP/b0.asm" -T "$TMP/b0.ld" -o "$TMP/x.cpr"
REASON="--cpr-bank 32 hors de 0..31" refuse "--cpr-bank 32" "$TMP/b0.asm" -T "$TMP/b0.ld" -o "$TMP/x.cpr" --cpr-bank 32
REASON=-E refuse ".cpr avec -E" "$TMP/b0.asm" -o "$TMP/x.cpr" --cpr-bank 0 -E
REASON="ne s'applique qu'a une sortie .cpr" refuse "--cpr-bank sans .cpr" "$TMP/b0.asm" -T "$TMP/b0.ld" -o "$TMP/x.bin" --cpr-bank 0

echo junk > "$TMP/bad.cpr"
REASON="ce n'est pas un conteneur CPR" refuse ".cpr existant mal forme" "$TMP/b0.asm" -T "$TMP/b0.ld" -o "$TMP/bad.cpr" --cpr-bank 0
[ "$(cat "$TMP/bad.cpr")" = junk ] || { echo "ECHEC : le .cpr mal forme a ete reecrit"; exit 1; }
echo "acceptation : les refus du .cpr tiennent, le fichier existant reste intact"
