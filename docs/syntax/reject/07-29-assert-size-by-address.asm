; expect-error: Block at &0100 exceeds its asserted size (0xA > 0x8 bytes)
; §7.assert_size — without a label, the block is designated by its address.
        org #100
        assert_size 8
        dw 1, 2, 3, 4, 5
        end_assert_size
