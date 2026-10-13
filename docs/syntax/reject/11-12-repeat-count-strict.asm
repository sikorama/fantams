; args: --strict
; expect-error: 'nop 32' is a writing facility, not canonical Z80
; §11 Repetition — `--strict` refuses a count.
    org #4000
    nop 32
