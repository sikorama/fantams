#!/bin/sh
# §15 Modes — --timing: durations in T-states and NOPs; an empty stretch lasts 0;
# empty fields and a note when a stretch has no exact duration; the last label
# of a block stays empty; one `assert` line per duration assert.
set -e
"$FANTAMS" main.asm --target cpc6128 -o "$TMP/a.bin" --timing="$TMP/t.csv"
grep -qx 'kind,name,file,line,tstates,nops,note' "$TMP/t.csv"
grep -qx 'label,entry,main.asm,3,0,0,' "$TMP/t.csv"
grep -qx 'label,start,main.asm,4,7,2,' "$TMP/t.csv"
grep -qx 'label,start.loop,main.asm,6,4,1,' "$TMP/t.csv"
grep -q "^label,wait,main.asm,8,,,\"main.asm:9 'jr z,start' has no fixed duration" "$TMP/t.csv"
grep -qx 'label,last,main.asm,10,,,' "$TMP/t.csv"
grep -qx 'assert,start..wait,main.asm,12,11,3,"tstates_between(start, wait) == 11 ok"' "$TMP/t.csv"
