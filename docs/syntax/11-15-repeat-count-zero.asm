; §11 Repetition — `nop 0` is legal and emits nothing; its label still names
; the address.
    org #4000
after:
    nop 0
    assert $ == #4000
    assert after == #4000
