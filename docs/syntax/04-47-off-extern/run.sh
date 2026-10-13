#!/bin/sh
# audio starts its bank: offset 0. Without EXTERN, the name is unknown.
set -e
"$FANTAMS" loader.asm --target cpc6128 -T map.ld -o "$TMP/l.bin"
got=$(od -An -tx1 -N 3 "$TMP/l.bin" | tr -d ' \n')
[ "$got" = "210000" ] || { echo "got $got, want 210000"; exit 1; }
grep -v extern loader.asm > "$TMP/noextern.asm"
if "$FANTAMS" "$TMP/noextern.asm" --target cpc6128 -T map.ld -o "$TMP/n.bin" >"$TMP/log" 2>&1; then
    echo "__off_ without EXTERN should be refused"; exit 1
fi
grep -qF "unknown symbol '__off_audio'" "$TMP/log"
