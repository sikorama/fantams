; expect-error: 'i' is a Z80 register or condition and cannot name a loop index
; §5.Reserved words — `i` is the Z80 interrupt register, so `for i = …` fails.
        for i = 0 to 1
        nop
        endfor
