; expect-error: unexpected character: '='
; §8 Diagnostics in the source — equality is `==`; `=` is an assignment and is
; refused in a condition.
        assert 1 = 1
