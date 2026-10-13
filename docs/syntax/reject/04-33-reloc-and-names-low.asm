; expect-error: use low() to take its low byte
; §4 Expressions, relocatable values — label & 255 is refused, naming low().
        section code, "ro"
here:
        ld   a, here & 255
