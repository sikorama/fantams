; expect-error: 'name' is declared EXTERN and also defined in this object
; §4 Expressions, scope between objects — declaring extern a name this object
; defines is refused.
        section code, "ro"
        extern name
name:
        ret
