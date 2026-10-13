; expect-sym: gfx.plot,label
; expect-sym: gfx.y,const
; §10.Modules — a module and a struct may share a name while no member name is the same.
        org 0
        MODULE gfx
plot:   nop
        ENDMODULE
        STRUCT gfx
y       db 0
        ENDSTRUCT
        assert sizeof(gfx) == 1
