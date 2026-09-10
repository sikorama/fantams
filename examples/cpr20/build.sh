#!/bin/sh
# build.sh — assemble et lie l'exemple cpr20 en un cpr20.cpr, via la CLI
# `fantams` elle-meme (« -o x.cpr --cpr-bank n », voir asm_main.cpp) : chaque
# banque physique est liee SEPAREMENT (cpr.h) et AJOUTEE au conteneur, une
# invocation par banque — comme la compilation separee ajoute un .fo a la fois.
set -e
cd "$(dirname "$0")"
FANTAMS=${FANTAMS:-../../fantams}
OUT=cpr20.cpr
rm -f "$OUT"

"$FANTAMS" boot.asm -T boot.ld -o "$OUT" --cpr-bank 0

./gen_banks.sh
for n in $(seq 1 20); do
    f=$(printf "bank%02d" "$n")
    "$FANTAMS" "$f.asm" -T "$f.ld" -o "$OUT" --cpr-bank "$n"
    rm -f "$f.asm" "$f.ld"
done

echo "ecrit : $(pwd)/$OUT"
