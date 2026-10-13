; expect-sym: mon_a,label
; expect-sym: b_fin,label
; §9.Scope — what the caller names is the caller's: a label whose name contains a
; substitution is global, wherever the braces are.
        org 0
macro fn(name)
mon_{name}:  nop
{name}_fin:  nop
endmacro
        fn(a)
        fn(b)
        jp mon_a
        assert b_fin - mon_a == 3
