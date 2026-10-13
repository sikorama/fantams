; expect-error: unknown macro 'a' — did you mean 'm.a'?
; §10.Modules — a module's macro called unqualified from outside is refused.
        MODULE m
macro a()
        nop
endmacro
        ENDMODULE
        a()
