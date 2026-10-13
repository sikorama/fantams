; expect-error: duplicate symbol: 'mon_a'
; §9.Scope — two calls that build the same name are a real collision.
macro fn(name)
mon_{name}:  nop
endmacro
        fn(a)
        fn(a)
