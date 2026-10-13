; expect-error: duplicate symbol: 'gfx.plot.x' — the same dotted name is defined two ways: as the local label '.x' of 'gfx.plot'
; §10.Modules — `.` is the only separator: one dotted name from two places is refused
; at its second definition, naming both sides.
        MODULE gfx
plot:   nop
.x:     nop
        ENDMODULE
        MODULE gfx
        MODULE plot
x:      nop
        ENDMODULE
        ENDMODULE
