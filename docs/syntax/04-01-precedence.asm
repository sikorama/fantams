; §4 Expressions, operators — the levels, from least to most binding.
; Each line pairs two neighbouring levels; the result shows which binds first.
        assert (0 && 1 || 1) == 1       ; && before ||
        assert (1 | 2 ^ 3) == 1         ; ^ before |
        assert (6 ^ 3 & 1) == 7         ; & before ^
        assert (3 & 1 == 1) == 1        ; == before &
        assert (0 == 2 < 1) == 1        ; < before ==
        assert (1 < 1 << 1) == 1        ; << before <
        assert (1 << 1 + 1) == 4        ; + before <<
        assert (2 + 3 * 4) == 14        ; * before +
        assert (7 mod 4) == 3 && (7 % 4) == 3
        assert (8 shr 1) == 4 && (1 shl 2) == 4
        assert (16 / 4 * 2) == 8        ; one level, left to right
