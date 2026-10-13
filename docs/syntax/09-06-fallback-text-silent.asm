; §9.Arguments — a register, `(ix+2)`, a string of more than one byte have no value:
; the bare form falls back to their text, without a warning.
        org 0
macro save(reg)
        push reg
endmacro
macro load(dst,src)
        ld dst,src
endmacro
macro text(s1)
        db s1
endmacro
start:  save(hl)
        load(a,(ix+2))
        text("hello")
        assert $ - start == 1 + 3 + 5
