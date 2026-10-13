; §9.Arguments — the argument is evaluated, never substituted textually:
; `m(1+1)` in a body doing `x*2` gives 4, not 1+1*2.
macro m(x)
v = x*2
endmacro
        m(1+1)
        assert v == 4
