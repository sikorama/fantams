; §4 Expressions, functions — sin and cos take radians; high and low (hi, lo)
; take a byte of the integer value; min and max take two arguments.
        assert sin(0) == 0 && cos(0) == 1
        assert sin(90 * 3.14159265 / 180) > 0.9999    ; a degree argument
        assert abs(-3) == 3
        assert high(#1234) == #12 && low(#1234) == #34
        assert hi(#1234) == high(#1234) && lo(#1234) == low(#1234)
        assert min(2, 3) == 2 && max(2, 3) == 3
