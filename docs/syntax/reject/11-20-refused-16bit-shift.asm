; expect-error: there is no 16-bit rotate/shift on the Z80
; expect-error: write a macro
; §11 What is refused — a 16-bit shift is a routine, not an orthography.
    org #4000
    rlc hl
