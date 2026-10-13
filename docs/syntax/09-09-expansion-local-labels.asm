; expect-sym: delay__1.loop,label
; expect-sym: delay__2.loop,label
; §9.Scope — a label written literally in a body is local to each expansion:
; the macro name and a call number prefix it, and two expansions never collide.
        org 0
macro delay()
loop:   djnz loop
endmacro
        delay()
        delay()
