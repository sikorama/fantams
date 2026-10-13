; §7 Placement — `align n` advances to the next multiple of `n`, emitting nothing.
        org #8001
        align 256
page:   nop
        assert page == #8100
