#!/bin/sh
# audio lands in ext1 (store 5), main in base1 (store 1).
set -e
"$FANTAMS" bankof.asm --target cpc6128 -T map.ld -o "$TMP/b.bin"
got=$(od -An -tx1 -N 3 "$TMP/b.bin" | tr -d ' \n')
[ "$got" = "050501" ] || { echo "bankof: got $got, want 050501"; exit 1; }
