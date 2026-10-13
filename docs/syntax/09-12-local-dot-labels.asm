; expect-sym: toto__1.loop,label
; expect-sym: toto__2.loop,label
; expect-sym: main.after,label
; §9.Scope — a local `.loop` in a body belongs to the expansion; the caller's own
; locals stay attached to the caller's last label.
        org 0
macro toto()
.loop:  djnz .loop
endmacro
main:   toto()
        toto()
.after: nop
