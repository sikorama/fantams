; expect-error: a relocatable value cannot carry a fractional offset
; §4 Expressions, relocatable values — an address is an integer.
        section code, "ro"
here:
        dw   here + 0.5
