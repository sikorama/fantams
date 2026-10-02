#!/bin/sh
# build.sh — assemble la meme ROM dans les slots 10 et 11 d'un demo.cro, via la
# CLI `fantams` elle-meme (« -o x.cro --cro-rom n », ADR 0033) : une invocation
# par ROM, chacune AJOUTEE au groupe 0 du conteneur.
set -e
cd "$(dirname "$0")"
FANTAMS=${FANTAMS:-../../fantams}
OUT=demo.cro
rm -f "$OUT"

for n in 10 11; do
    "$FANTAMS" demo.asm -T "slot$n.ld" -o "$OUT" --cro-rom "$n" --cro-label "Demo ROMs"
done

echo "ecrit : $(pwd)/$OUT"
