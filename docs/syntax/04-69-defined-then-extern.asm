; expect-error: 'name' is declared EXTERN and also defined in this object
; §4 Expressions, scope between objects — the same mistake in the other order.
        section code, "ro"
name:
        ret
        extern name
