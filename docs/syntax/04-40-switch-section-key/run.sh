#!/bin/sh
# audio sits in ext_w1<1>: ld bc,&7FC5 / out (c),c.
set -e
"$FANTAMS" switch.asm --target cpc6128 -T map.ld -o "$TMP/s.bin"
got=$(od -An -tx1 -N 5 "$TMP/s.bin" | tr -d ' \n')
[ "$got" = "01c57fed49" ] || { echo "got $got, want 01c57fed49"; exit 1; }
