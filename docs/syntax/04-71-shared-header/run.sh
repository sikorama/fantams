#!/bin/sh
set -e
"$FANTAMS" lib.asm -o "$TMP/lib.fo"
"$FANTAMS" user.asm -o "$TMP/user.fo"
"$FANTAMS" "$TMP/lib.fo" "$TMP/user.fo" -o "$TMP/p.bin"
