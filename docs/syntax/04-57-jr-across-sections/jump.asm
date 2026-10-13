; §4 Expressions, names the linker owns — a jr that crosses sections is
; measured by the linker, and refused there if out of range.
        section one, "ro"
        jr   far
        ds   200
        section two, "ro"
far:
        nop
