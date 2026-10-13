; §11 Repetition — every operand-less mnemonic takes a count, no blocklist.
    org #4000
a1:
    ldir 3
a2:
    di 2
a3:
    exx 2
a4:
    cpi 4
    assert a2 - a1 == 6
    assert a3 - a2 == 2
    assert a4 - a3 == 2
    assert $ - a4 == 8
