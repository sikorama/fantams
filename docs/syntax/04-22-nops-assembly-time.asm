; args: --target cpc6128
; §4 Expressions, nops_between() — known at assembly time, like a label: in an
; operand, an equ, a '=' variable, an assert, with 'to' defined below; 'ds' reserves
; space measured on labels above.
        org #4000
first:
        ld   a, nops_between(first, second)     ; 'second' is below
second:
        nop
cost    equ nops_between(first, second)
ticks = tstates_between(first, second)
        assert cost == 2 && ticks == 7
        ds   nops_between(first, second)
