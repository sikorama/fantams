; expect-error: unexpected character: '?'
; §4 Expressions, what doesn't exist — no ternary: the ':' splits '1 ? 2 : 3'
; before it reaches the evaluator. An 'if' says the same thing.
x = 1 ? 2 : 3
