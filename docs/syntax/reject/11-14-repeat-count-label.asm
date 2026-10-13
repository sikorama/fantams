; expect-error: a repetition count is a preprocessor value
; expect-error: use 'ds' instead
; §11 Repetition — a count touching a label is refused: at preprocessor time no
; address exists. The message points to `ds`.
    org #4000
first:
    nop
last:
    nop last-first
