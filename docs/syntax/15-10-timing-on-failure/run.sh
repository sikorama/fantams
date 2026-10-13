#!/bin/sh
# §15 Modes — --timing is written even when assembly fails.
if "$FANTAMS" main.asm -o "$TMP/a.bin" --timing="$TMP/t.csv" >"$TMP/log" 2>&1; then
    echo "should have failed"; exit 1
fi
grep -qx 'assert,start..stop,main.asm,6,7,,"tstates_between(start, stop) == 8 FAILED"' "$TMP/t.csv"
