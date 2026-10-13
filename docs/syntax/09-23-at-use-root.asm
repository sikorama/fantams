; §9.Scope — `@` on a use anchors the name at the root: `@CONST` is the global
; constant, `@toto.CONST` the full path, for labels, constants, structs and macro calls.
        org 0
CONST   EQU 1000
        STRUCT T
f1      db 0
        ENDSTRUCT
start:  nop
        MODULE toto
CONST   EQU 10
        STRUCT T
g1      dw 0
        ENDSTRUCT
start:  nop
macro m()
v = 7
endmacro
        assert CONST == 10
        assert @CONST == 1000
        assert @toto.CONST == 10
        assert start - @start == 1
        assert sizeof(T) == 2
        assert sizeof(@T) == 1
        @toto.m()
        ENDMODULE
        assert v == 7
