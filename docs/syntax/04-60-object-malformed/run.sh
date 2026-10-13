#!/bin/sh
set -e
"$FANTAMS" unit.asm -o "$TMP/a.fo"
sed 's/data 0 C9/data 0 ZZ/' "$TMP/a.fo" > "$TMP/bad.fo"
if "$FANTAMS" "$TMP/bad.fo" -o "$TMP/bad.bin" >"$TMP/log" 2>&1; then
    echo "a malformed object should be refused"; exit 1
fi
grep -qE 'bad.fo: line [0-9]+: data:' "$TMP/log"
