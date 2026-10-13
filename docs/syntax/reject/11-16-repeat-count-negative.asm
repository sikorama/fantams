; expect-error: 'nop -1': repetition count out of range
; §11 Repetition — a negative count is an error.
    org #4000
    nop -1
