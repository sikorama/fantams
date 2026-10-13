; args: --target cpc6128
; §4 Expressions, names the linker owns — __mask_<axis> names the bits of the
; port that belong to an axis, and __val_ is bounded to them, so a source can
; write (state & ~mask) | val without touching the neighbouring axes.
        assert __mask_rom_lower == %00000100
        assert __mask_rom_upper == %00001000
        assert __val_on == 0
state = #8C
        assert ((state & ~__mask_rom_lower) | __val_on) == #88
