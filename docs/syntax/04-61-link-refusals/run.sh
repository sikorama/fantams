#!/bin/sh
set -e
for f in needs f1 f2 at1 at2 entry2; do "$FANTAMS" $f.asm -o "$TMP/$f.fo"; done
refused() {  # refused <text> <objects...>
    text=$1; shift
    if "$FANTAMS" "$@" -o "$TMP/out.bin" >"$TMP/log" 2>&1; then
        echo "should be refused: $*"; exit 1
    fi
    grep -qF "$text" "$TMP/log" || { echo "missing: $text"; cat "$TMP/log"; exit 1; }
}
refused "unresolved EXTERN symbol 'missing'" "$TMP/needs.fo"
refused "symbol 'f' is exported twice" "$TMP/f1.fo" "$TMP/f2.fo"
refused "two objects write to &0100" "$TMP/at1.fo" "$TMP/at2.fo"
refused "two objects declare an entry point" "$TMP/at1.fo" "$TMP/entry2.fo"
