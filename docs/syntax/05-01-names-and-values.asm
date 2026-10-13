; §5 Names and values — a label is an address, EQU a constant, `=` a sequential
; variable, LET a variable the preprocessor must resolve on the spot.
        org #8000
start:  nop
K       EQU 4
v = 1
v = v + 1
LET n = K * 2
        assert start == #8000
        assert K == 4
        assert v == 2
        assert n == 8
