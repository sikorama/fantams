; §11 Repetition — the count is a preprocessor value: a variable or an
; expression is fine.
    org #4000
n = 3
start:
    ldi n*2
    assert $ - start == 12
