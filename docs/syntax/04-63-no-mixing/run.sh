#!/bin/sh
set -e
"$FANTAMS" a.asm -o "$TMP/a.fo"
if "$FANTAMS" a.asm "$TMP/a.fo" -o "$TMP/x.bin" >/dev/null 2>&1; then
    echo "mixing a source and an object should be refused"; exit 1
fi
