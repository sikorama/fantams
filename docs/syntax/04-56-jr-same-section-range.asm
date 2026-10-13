; expect-error: relative jump out of range (-128..127)
; §4 Expressions, names the linker owns — a jr whose target is in the same
; section is measured while assembling, and refused there if out of range.
        section code, "ro"
        jr   far
        ds   200
far:
        nop
