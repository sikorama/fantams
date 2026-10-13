; §10.Modules — modules nest; a name the inner module defines hides the outer one,
; a name only an enclosing module (or the root) defines stays visible bare.
ADDR    EQU 1
G       EQU 7
        MODULE toto
ADDR    EQU 2
OUTER   EQU 3
        MODULE chose
ADDR    EQU 4
        assert ADDR == 4
        assert OUTER == 3
        assert G == 7
        ENDMODULE
        assert ADDR == 2
        ENDMODULE
        assert toto.chose.ADDR == 4
        assert toto.ADDR == 2
        assert ADDR == 1
