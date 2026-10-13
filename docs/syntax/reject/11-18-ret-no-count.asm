; expect-error: unrecognized RET form
; §11 Repetition — `ret` is not operand-less (`ret z`), so `ret 2` is not a count.
    org #4000
    ret 2
