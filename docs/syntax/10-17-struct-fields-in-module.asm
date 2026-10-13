; §10.Modules — struct fields are qualified by their struct, never by the module;
; inside the module a name is bare, outside it is qualified.
        org 0
        MODULE gfx
        STRUCT S_COLORS
HUD     db 0
BG      db 0
        ENDSTRUCT
        assert sizeof(S_COLORS) == 2
        assert S_COLORS.BG == 1
        ENDMODULE
        STRUCT gfx.S_COLORS inst
        assert sizeof(gfx.S_COLORS) == 2
        assert gfx.S_COLORS.HUD == 0
        assert inst.BG == inst + 1
