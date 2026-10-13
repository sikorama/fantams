; §4 Expressions, relocatable values — high() and low() take the byte of a
; relocatable address, resolved at link time.
        section code, "ro"
here:
        ld   a, high(here)
        ld   b, low(here)
