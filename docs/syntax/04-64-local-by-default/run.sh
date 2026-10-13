#!/bin/sh
set -e
"$FANTAMS" a.asm -o "$TMP/a.fo"
"$FANTAMS" b.asm -o "$TMP/b.fo"
"$FANTAMS" "$TMP/a.fo" "$TMP/b.fo" -o "$TMP/p.bin"
