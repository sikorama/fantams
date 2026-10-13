; expect-error: Section 'vars' exceeds maximum declared size (0x5 > 0x4 bytes)
; §7."uninit" — reserved space counts towards the declared maximum.
        section vars, "uninit", 4
        org #C000
        ds 5
