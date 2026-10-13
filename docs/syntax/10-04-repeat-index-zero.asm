; expect-warning: the index of 'repeat' starts at 0
; §10.Blocks — the `repeat` index starts at 0, which is what `idx*8` next to it expects.
sum = 0
        repeat 3,idx
sum = sum + idx*8
        endrepeat
        assert sum == 0 + 8 + 16
