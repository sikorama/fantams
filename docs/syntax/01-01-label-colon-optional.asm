; expect-warning: label without ':': 'start'
; §1 The line — the colon of a label is optional, but its absence is flagged.
        org #4000
start
        nop
        assert start == #4000
