; expect-error: a string literal of 2 bytes has no value
; §3 Strings — a string of any other length has no value in an expression.
        ld hl, 'ab'
