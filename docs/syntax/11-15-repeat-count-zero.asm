; §11 Repetition — `nop 0` is legal and emits nothing; its label still names
; the address.
    org #4000
fin:
    nop 0
    assert $ == #4000
    assert fin == #4000
