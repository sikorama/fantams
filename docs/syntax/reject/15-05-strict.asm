; args: --strict
; expect-error: several instructions on one line is a writing facility, not canonical Z80
; §15 Modes — `--strict` refuses anything that is not canonical Z80.
    org #4000
    ld a,1 : inc a
