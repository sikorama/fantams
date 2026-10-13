; §4 Expressions, separate assembly — what the linker refuses across objects:
; an extern nobody exports, a name exported twice, two objects writing one
; address, two entry points.
        section c1, "ro"
        extern missing
        call missing
