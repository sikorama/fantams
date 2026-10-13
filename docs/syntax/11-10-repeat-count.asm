; args: -E
; expect-out: ldi
; §11 Repetition — an operand-less mnemonic takes a count, shorthand for
; `repeat n : <mnemonic> : endrepeat`; `-E` unrolls it fully.
    org #4000
pad:
    nop 32
copy:
    ldi 16
stop:
    halt 2
    assert copy - pad == 32
    assert stop - copy == 32
    assert $ - stop == 2
