; §15 Modes — a failed duration assert fails assembly, and the table is written.
    org #4000
start:
    ld a,5
stop:
    assert tstates_between(start, stop) == 8
