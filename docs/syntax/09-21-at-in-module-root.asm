; expect-sym: x,label
; §9.Scope — inside a module, `@x:` defines `x` at the root.
        org 0
        MODULE a
@x:     nop
y:      nop
        ENDMODULE
        assert a.y - x == 1
