; expect-error: duplicate symbol: 'my_a'
; §9.Scope — two calls that build the same name are a real collision.
macro fn(name)
my_{name}:  nop
endmacro
        fn(a)
        fn(a)
