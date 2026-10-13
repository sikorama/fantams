; §9.Arguments — the bare form is the value captured at the call (5, 5, 5);
; braces are the text, resolved at the emission point (5, 4, 3).
bare = 0
text = 0
n = 5
macro m x
    repeat 3
bare = bare + x
text = text + {x}
n = n - 1
    endrepeat
endmacro
        m(n)
        assert bare == 5+5+5
        assert text == 5+4+3
