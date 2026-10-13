; expect-error: SECTION 'tab' was already declared with a maximum size of 0x4
; §7.A declared maximum size — a reopening may omit the maximum, not change it.
        section tab, "ro", 4
        org #4000
        db 1
        section tab, "ro", 8
