; §2 Numbers — every value is a real; a bitwise operator converts its operands
; to integer first, rounding: 7/2 is 3.5, and (7/2) | 0 is 4.
        assert 7 / 2 == 3.5
        assert ((7 / 2) | 0) == 4
