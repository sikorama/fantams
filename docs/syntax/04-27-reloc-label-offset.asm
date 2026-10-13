; §4 Expressions, relocatable values — in a section with no org of its own,
; label, label + 4 and label - 4 stay relocatable.
        section code, "ro"
here:
        dw   here, here + 4, here - 4
