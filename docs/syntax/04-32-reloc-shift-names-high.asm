; expect-error: '>>' cannot be applied to a relocatable value: its address is not known until link time; use high()
; §4 Expressions, relocatable values — label >> 8 is refused, naming high().
        section code, "ro"
here:
        ld   a, here >> 8
