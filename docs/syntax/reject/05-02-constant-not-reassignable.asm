; expect-error: duplicate symbol: 'K' — already defined as a constant
; §5 Names and values — a constant is not reassignable: a second EQU is refused.
K       EQU 1
K       EQU 2
