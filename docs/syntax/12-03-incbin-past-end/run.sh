#!/bin/sh
# §12 Inclusion — incbin past the end of the file is refused.
if "$FANTAMS" main.asm -o "$TMP/a.bin" >"$TMP/log" 2>&1; then
    echo "should have failed"; exit 1
fi
grep -qF "INCBIN: offset+length 41 beyond end of 'data.bin' (40 bytes)" "$TMP/log"
