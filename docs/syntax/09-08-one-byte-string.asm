; §9.Arguments — a string of one byte has a value: `m('A')` passes 65.
macro m(x)
v = x
endmacro
        m('A')
        assert v == 65
