; args: --target cpc6128
; §4 Expressions, nops_between() and tstates_between() — the duration of the
; code from one label up to, not including, another: NOPs summed per
; instruction from the table, so 47 T-states is 14 NOPs here, not 12.
        org #4000
start:
        ld   a, 5           ; 7 T-states  -> 2 NOPs
        ld   (#4000), a     ; 13          -> 4
        ld   hl, (#4000)    ; 16          -> 5
        inc  (hl)           ; 11          -> 3
stop:
        nop
        assert nops_between(start, stop) == 14
        assert tstates_between(start, stop) == 47
