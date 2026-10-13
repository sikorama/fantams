; §9.Scope — a variable is never scoped: it stays the accumulator its caller reads.
n = 0
macro bump()
n = n + 1
endmacro
        bump()
        bump()
        assert n == 2
