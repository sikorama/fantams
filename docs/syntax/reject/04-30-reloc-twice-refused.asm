; expect-error: a relocatable expression is ONE section base plus an offset
; §4 Expressions, relocatable values — label + label has no meaning.
        section code, "ro"
here:
        dw   here + here
