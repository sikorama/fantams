#!/bin/sh
set -e
for f in lib user wrong; do "$FANTAMS" $f.asm -o "$TMP/$f.fo"; done
"$FANTAMS" "$TMP/lib.fo" "$TMP/user.fo" -o "$TMP/ok.bin"
if "$FANTAMS" "$TMP/lib.fo" "$TMP/wrong.fo" -o "$TMP/ko.bin" >"$TMP/log" 2>&1; then
    echo "extern init should fail at link time"; exit 1
fi
grep -qF "unresolved EXTERN symbol 'init'" "$TMP/log"
