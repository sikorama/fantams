; expect-sym: my_a,label
; expect-sym: b_end,label
; §9.Scope — what the caller names is the caller's: a label whose name contains a
; substitution is global, wherever the braces are.
        org 0
macro fn(name)
my_{name}:  nop
{name}_end:  nop
endmacro
        fn(a)
        fn(b)
        jp my_a
        assert b_end - my_a == 3
