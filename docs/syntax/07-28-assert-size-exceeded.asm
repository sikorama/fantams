; expect-error: Block 'jump_table' exceeds its asserted size (0xA > 0x8 bytes)
; §7.assert_size — a cap on a sub-area, named by its first label, reported on
; the `assert_size` line.
        org #100
draw:   nop
move:   nop
hide:   nop
kill:   nop
wait:   nop
        assert_size 8
jump_table:
        dw draw, move, hide, kill, wait
        end_assert_size
