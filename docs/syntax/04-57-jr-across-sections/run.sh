#!/bin/sh
# The object assembles; the link refuses the distance.
set -e
"$FANTAMS" jump.asm -o "$TMP/j.fo"
if "$FANTAMS" "$TMP/j.fo" -o "$TMP/j.bin" >"$TMP/log" 2>&1; then
    echo "link should have failed"; exit 1
fi
grep -qF "the target is in another section, and the distance is only known here" "$TMP/log"
