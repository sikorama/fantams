; expect-error: a relocatable address does not fit in one byte: use high() or low()
; §4 Expressions, relocatable values — db label is refused.
        section code, "ro"
here:
        db   here
