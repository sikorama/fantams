#!/bin/sh
# §7.Banks — the .sna carries 64 KB within banks 0-3, 128 KB with banks 4-7,
# and export refuses beyond bank 7, naming the bank.
set -e
"$FANTAMS" base.asm -o "$TMP/base.sna" >/dev/null 2>&1
"$FANTAMS" ext.asm -o "$TMP/ext.sna" >/dev/null 2>&1
[ "$(wc -c < "$TMP/base.sna")" -eq $((256 + 65536)) ]
[ "$(wc -c < "$TMP/ext.sna")" -eq $((256 + 131072)) ]
if "$FANTAMS" beyond.asm -o "$TMP/beyond.sna" >"$TMP/log" 2>&1; then exit 1; fi
grep -qF "bank(s) 8 written, beyond bank 7" "$TMP/log"
