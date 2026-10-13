; expect-sym: buffer,label,vars,0xC000,
; expect-sym: flag,label,vars,0xC010,
; §7."uninit" — `ds` advances the address without writing: the labels exist,
; the binary holds only the code section.
        section vars, "uninit"
        org #C000
buffer: ds 16
flag:   ds 1

        section code, "ro"
        org #8000
start:  ld a, (flag)
past:
        assert flag == #C010
        assert past - start == 3
