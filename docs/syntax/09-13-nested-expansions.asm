; expect-sym: m__1.repeat__1.x,label
; expect-sym: m__1.repeat__2.x,label
; expect-sym: a.m__2.repeat__3.x,label
; §9.Scope — expansions nest, and inside a module the module prefix comes first.
        org 0
macro m()
        repeat 2
x:      nop
        endrepeat
endmacro
        m()
        MODULE a
        m()
        ENDMODULE
