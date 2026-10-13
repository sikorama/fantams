; expect-error: '*' cannot be applied to a relocatable value
; §4 Expressions, relocatable values — label * 2 has no meaning.
        section code, "ro"
here:
        dw   here * 2
