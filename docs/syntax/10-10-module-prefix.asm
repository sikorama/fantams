; expect-sym: gfx.plot,label
; expect-sym: gfx.S_COLORS,const
; expect-sym: gfx.cur,label
; expect-sym: gfx.K,const
; §10.Modules — labels, constants, structs and struct instances of a module are prefixed.
        org 0
        MODULE gfx
K       EQU 3
        STRUCT S_COLORS
HUD     db 0
        ENDSTRUCT
plot:   nop
        STRUCT S_COLORS cur
        ENDMODULE
        call gfx.plot
