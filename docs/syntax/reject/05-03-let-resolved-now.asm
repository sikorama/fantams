; expect-error: unknown symbol 'later'
; §5 Names and values — LET is resolved by the preprocessor, so it cannot read a
; name defined further down.
LET n = later
later   EQU 3
