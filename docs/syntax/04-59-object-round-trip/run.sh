#!/bin/sh
set -e
"$FANTAMS" unit.asm -o "$TMP/a.fo"
"$FANTAMS" "$TMP/a.fo" -o "$TMP/b.fo"
cmp "$TMP/a.fo" "$TMP/b.fo"
head -1 "$TMP/a.fo" | grep -q '^fantams-object'
