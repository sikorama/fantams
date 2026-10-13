#!/bin/sh
# §12 Inclusion — incbin: whole file, offset+length, offset to the end; the
# unrolled source carries the bytes as `db` lines, 16 per line, and reassembles
# without the file.
set -e
"$FANTAMS" main.asm -o "$TMP/a.bin"
"$FANTAMS" main.asm -E -o "$TMP/a.pp.asm"
grep -q 'db #00,#01,#02,#03,#04,#05,#06,#07,#08,#09,#0A,#0B,#0C,#0D,#0E,#0F$' "$TMP/a.pp.asm"
grep -q 'db #04,#05,#06,#07,#08$' "$TMP/a.pp.asm"
if grep -q incbin "$TMP/a.pp.asm"; then echo "incbin left in -E"; exit 1; fi
# Reassembled away from data.bin: same bytes.
"$FANTAMS" "$TMP/a.pp.asm" -o "$TMP/b.bin"
cmp "$TMP/a.bin" "$TMP/b.bin"
