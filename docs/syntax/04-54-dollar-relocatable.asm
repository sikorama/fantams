; §4 Expressions, names the linker owns — '$' behaves as a label of the current
; section, so the idioms built on it cross relocation unchanged.
        section code, "ro"
start:
        dw   $
        db   high($), $ - start
        assert $ - start == 4
