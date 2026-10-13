; expect-error: sizeof: unknown struct 'S'
; §10.Modules — outside its module a struct is written qualified, with no fallback search.
        MODULE gfx
        STRUCT S
A1      db 0
        ENDSTRUCT
        ENDMODULE
        assert sizeof(S) == 1
