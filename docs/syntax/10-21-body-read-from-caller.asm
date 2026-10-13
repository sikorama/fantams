; §10.Modules — a macro body is read from the caller: called in `main`, `CONST` is
; `main.CONST`; `@CONST` reaches the global one.
CONST   EQU 1000
macro rd()
seen = CONST
root = @CONST
endmacro
        MODULE main
CONST   EQU 10
        rd()
        ENDMODULE
        assert seen == 10
        assert root == 1000
