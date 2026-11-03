#!/bin/sh
# accept_dsk.sh — l'export disquette, par la CLI reelle (ADR 0007).
#
# pack_test verifie le decoupage, les en-tetes AMSDOS et la fusion sur des
# images fabriquees en memoire. Ce qui ne se verifie qu'ici, c'est le
# branchement :
#
#   1. « -o x.dsk » choisit la disquette sur l'extension, de bout en bout, pour
#      un programme a un `org` et pour le programme banque de l'exemple ;
#   2. deux invocations sur le meme .dsk y laissent les deux programmes ;
#   3. une disquette existante est intacte apres un export en erreur ;
#   4. les options sans objet pour une disquette sont refusees, --sym passe.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

OUT="$TMP/prog.dsk"
sig() { head -c 8 "$1"; }
# Une entree de repertoire : nom sur 8 + extension sur 3, en clair dans l'image.
has() { grep -a -q -- "$2" "$1"; }

# 1. Un org : un fichier nomme d'apres le source.
"$FANTAMS" examples/demo.asm -o "$OUT" >"$TMP/log" 2>&1 ||
    { echo "ECHEC : demo.asm -> .dsk refuse"; cat "$TMP/log"; exit 1; }
[ "$(sig "$OUT")" = "EXTENDED" ] || { echo "ECHEC : $OUT n'est pas une disquette EXTENDED"; exit 1; }
has "$OUT" "DEMO    BIN" || { echo "ECHEC : DEMO.BIN absent de la disquette"; exit 1; }
echo "acceptation : demo.asm donne DEMO.BIN sur une disquette"

# 2. Le programme banque, AJOUTE sur la meme disquette : un fichier par
#    emplacement, et DEMO.BIN toujours la.
"$FANTAMS" examples/banked.asm -T examples/banked.ld -o "$OUT" >"$TMP/log" 2>&1 ||
    { echo "ECHEC : banked.asm -> .dsk refuse"; cat "$TMP/log"; exit 1; }
names=$(grep -a -o 'B[0-9A-F][0-9A-F]_[0-9A-F][0-9A-F][0-9A-F][0-9A-F]BIN' "$OUT" | sort -u | tr '\n' ' ')
[ "$names" = "B01_4000BIN B02_8000BIN B05_4000BIN " ] ||
    { echo "ECHEC : fichiers [$names], attendu B01_4000, B02_8000 et B05_4000"; exit 1; }
has "$OUT" "DEMO    BIN" || { echo "ECHEC : DEMO.BIN a disparu en ajoutant le programme banque"; exit 1; }
echo "acceptation : banked.asm ajoute B01_4000, B02_8000 et B05_4000, DEMO.BIN reste"

# 3. Une disquette illisible n'est pas touchee par un export en erreur.
printf 'pas une disquette' >"$TMP/bad.dsk"
cp "$TMP/bad.dsk" "$TMP/bad.orig"
if "$FANTAMS" examples/demo.asm -o "$TMP/bad.dsk" >"$TMP/err" 2>&1; then
    echo "ECHEC : une disquette illisible devrait etre refusee"; exit 1
fi
cmp -s "$TMP/bad.dsk" "$TMP/bad.orig" || { echo "ECHEC : la disquette illisible a ete reecrite"; exit 1; }
echo "acceptation : une disquette illisible est refusee et laissee intacte"

# 4. Les refus, et --sym.
refuse() {
    what=$1; shift
    if "$FANTAMS" "$@" >"$TMP/err" 2>&1; then
        echo "ECHEC : $what devrait etre refuse"; exit 1
    fi
    grep -q -- "$REASON" "$TMP/err" || { echo "ECHEC : $what : le message ne nomme pas '$REASON'"; cat "$TMP/err"; exit 1; }
}
REASON=--base refuse ".dsk avec --base" examples/demo.asm -o "$TMP/x.dsk" --base "$TMP/b.sna"
REASON=--cpr-bank refuse ".dsk avec --cpr-bank" examples/demo.asm -o "$TMP/x.dsk" --cpr-bank 0
REASON=--cro-rom refuse ".dsk avec --cro-rom" examples/demo.asm -o "$TMP/x.dsk" --cro-rom 7
# -E, --beautify ou --normalize ecriraient du texte a la place de la disquette.
REASON=-E refuse ".dsk avec -E" examples/demo.asm -o "$TMP/x.dsk" -E
"$FANTAMS" examples/demo.asm -o "$TMP/s.dsk" --sym="$TMP/s.sym" >"$TMP/log" 2>&1 ||
    { echo "ECHEC : --sym refuse avec .dsk"; cat "$TMP/log"; exit 1; }
[ -s "$TMP/s.sym" ] || { echo "ECHEC : --sym n'a rien ecrit"; exit 1; }
echo "acceptation : --base, --cpr-bank, --cro-* et -E refuses, --sym accepte"
