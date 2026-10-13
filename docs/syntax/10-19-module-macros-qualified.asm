; §10.Modules — a macro of `MODULE m` is `m.a`: `a()` inside (innermost module, then
; enclosing ones, then global), `m.a()` outside. Two modules each have their own `a`.
w = 0
macro a()
w = w + 1
endmacro
        MODULE m
macro a()
w = w + 10
endmacro
        MODULE inner
        a()                     ; m.a, found in the enclosing module
        ENDMODULE
        a()                     ; m.a
        ENDMODULE
        MODULE n
macro a()
w = w + 100
endmacro
        ENDMODULE
        m.a()
        n.a()
        a()                     ; the global one
        assert w == 10 + 10 + 10 + 100 + 1
