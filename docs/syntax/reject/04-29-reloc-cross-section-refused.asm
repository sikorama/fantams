; expect-error: two relocatable values from different sections cannot be combined
; §4 Expressions, relocatable values — a distance between two sections is the
; linker's to decide.
        section one, "ro"
here:
        nop
        section two, "ro"
there:
        dw   there - here
