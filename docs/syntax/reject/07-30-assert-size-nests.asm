; expect-error: Block 'inner' exceeds its asserted size (0x3 > 0x2 bytes)
; §7.assert_size — blocks nest, unlike `boundary`: the inner one is measured too.
        org #100
        assert_size 10
outer:  db 1
        assert_size 2
inner:  db 2, 3, 4
        end_assert_size
        end_assert_size
