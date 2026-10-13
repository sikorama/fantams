; §15 Modes — `--timing`: one line per label, its duration up to the next label.
    org #4000
entry:
start:
    ld a,5
.loop:
    nop
wait:
    jr z,start
last:
    ret
    assert tstates_between(start, wait) == 11
