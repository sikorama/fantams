#!/bin/sh
set -e
"$FANTAMS" unit.asm -o "$TMP/u.fo"
"$FANTAMS" "$TMP/u.fo" --target cpc6128 -T map.ld -o "$TMP/u.bin"
got=$(od -An -tx1 -N 8 "$TMP/u.bin" | tr -d ' \n')
[ "$got" = "01007f067f21c500" ] || { echo "got $got, want 01007f067f21c500"; exit 1; }
