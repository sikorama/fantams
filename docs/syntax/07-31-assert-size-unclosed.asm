; expect-error: ASSERT_SIZE without a matching END_ASSERT_SIZE
; §7.assert_size — a missing `end_assert_size` is refused.
        org #100
        assert_size 4
        nop
