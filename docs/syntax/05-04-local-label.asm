; §5.Local labels — `.x` belongs to the last global label; it is reached from
; outside as `plot.x`. An interleaved definition does not change the owner.
        org #8000
plot:
.x:     ld a,0
        ld (plot.x+1),a
delta   equ 4
.y:     nop
        assert plot.x == #8000
        assert .y == plot.y
        assert plot.y == #8005
