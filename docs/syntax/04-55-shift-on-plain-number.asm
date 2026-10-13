; §4 Expressions, names the linker owns — '>> 8' and '& 255' stay legal on a
; plain number.
        assert (#1234 >> 8) == #12
        assert (#1234 & 255) == #34
        ld   a, #1234 >> 8
