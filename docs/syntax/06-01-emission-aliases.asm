; §6 Emission directives — db/defb/dm/defm emit bytes and strings, dw/defw
; little-endian words, ds/defs/rmb reserve `n` bytes with value `v`.
        org #100
b0:     db 1,"AB"
b1:     defb 2
        dm "C"
        defm "D"
w0:     dw #1234
        defw #5678
s0:     ds 2,#AA
        defs 1,#BB
        rmb 1,#CC
stop:
        assert b1 - b0 == 3
        assert s0 - w0 == 4
        assert stop - s0 == 4
