; §9.Scope — a name the body defines hides the same name outside; a name it does not
; define (here `table`) stays visible as written.
        org 0
start:  nop
table:  db 1
macro m()
start:  nop
        assert start - @start == 2      ; `start` is m__1.start, `@start` the global one
        assert table == 1
endmacro
        m()
