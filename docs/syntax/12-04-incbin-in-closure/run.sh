#!/bin/sh
# §12 Inclusion — `--list-files` prints the source and the incbin file.
set -e
"$FANTAMS" main.asm --list-files >"$TMP/files"
grep -qx 'main.asm' "$TMP/files"
grep -qx 'data.bin' "$TMP/files"
