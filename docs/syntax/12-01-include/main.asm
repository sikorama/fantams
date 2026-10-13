; §12 Inclusion — `include "file"` inserts a source: its names are defined here.
    org #4000
    include "consts.asm"
start:
    ld a,WIDTH
    assert WIDTH == 40
    assert $ - start == 2
