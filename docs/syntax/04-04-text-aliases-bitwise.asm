; §4 Expressions, operators — the text aliases are the bitwise forms: 'not' is
; '~', not '!'. '&&', '||' and '!' have no text alias.
        assert not 0 == -1
        assert ~0 == -1
        assert !0 == 1
        assert (6 and 3) == 2 && (6 or 1) == 7 && (6 xor 3) == 5
        assert (2 and 1) == 0 && (2 && 1) == 1
