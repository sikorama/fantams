; expect-error: section 'vars' is "uninit": it reserves space and emits no bytes
; §7."uninit" — db, dw, a string or an instruction is refused there.
        section vars, "uninit"
        org #C000
        db 1
