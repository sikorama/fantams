#!/bin/sh
# gen_banks.sh — engendre bankNN.asm + bankNN.ld : 20 banques d'un seul
# tenant, chacune 16 Ko d'un motif simple et DIFFERENT.
#
# Non commis : reproductible en une commande, comme les .fo qu'ils produisent.
set -e
cd "$(dirname "$0")"

for n in $(seq 1 20); do
    f=$(printf "bank%02d" "$n")
    inv=$((255 - n))
    cat > "$f.asm" <<EOF
        SECTION cart, "ro"
cart:
        REPEAT 8192
        DB $n, $inv
        REND
EOF
    cat > "$f.ld" <<EOF
TARGET cpcplus
MEMORY_MAP { CONFIG cart_rom_hi.on<$n> { w3 { SECTION cart } } }
EOF
done
echo "20 banques engendrees (bank01.asm/.ld .. bank20.asm/.ld)"
