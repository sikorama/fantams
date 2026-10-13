#!/bin/sh
# §7.Sections — a link script that places the same section wins, and two
# warnings say so: one on the IN, one on the script line.
set -e
"$FANTAMS" main.asm -T place.ld -o "$TMP/out.sna" --sym="$TMP/out.sym" >"$TMP/log" 2>&1
grep -qF "and the script places it too: the script wins" "$TMP/log"
grep -qF "place.ld:3: warning: this is the placement of section 'gfx' that wins" "$TMP/log"
grep -qF "data,label,gfx,0x4000,6," "$TMP/out.sym"
