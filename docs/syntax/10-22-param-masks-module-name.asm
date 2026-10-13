; §10.Modules — a parameter hides a name of the caller's module: `len` is the argument,
; bare or in braces, not `mod.len`.
macro put(len)
bare = len
text = {len}
endmacro
        MODULE mod
len     EQU 9
        put(3)
        ENDMODULE
        assert bare == 3
        assert text == 3
