; args: -E --fold
; expect-out: db 2
; expect-out: db 4
; expect-out: v=4
; §15 Modes — `-E --fold` writes what the assembler receives: operands resolvable
; at preprocessor time replaced by their value, only the last assignment kept.
    org #4000
v = 1
    repeat 2
v = v*2
    db v
    endrepeat
