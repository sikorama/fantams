; expect-error: 'p' is a Z80 register or condition and cannot name a macro parameter
; §5.Reserved words — `p` is a condition (sign positive): no parameter named `p`.
macro put(p)
        db p
endmacro
