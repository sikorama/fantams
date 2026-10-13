; §4 Expressions, functions — floor, ceil, int, round go toward -inf, +inf,
; zero, nearest; halves round up.
        assert floor(-3.5) == -4
        assert ceil(-3.5) == -3
        assert int(-3.5) == -3
        assert round(3.5) == 4
        assert round(-3.5) == -3
