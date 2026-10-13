; expect-sym: wait__1.loop,label
; expect-sym: repeat__1.r,label
; §5.Local labels — a label written in an expansion body (macro, repeat, while,
; for) is local to that expansion: it does not change the owner of `.x`.
        org #8000
macro wait()
loop:   djnz loop
endmacro
main:
.loop:  nop
        wait()
        repeat 1
r:      nop
        endrepeat
        jp .loop
.after: nop
        assert .after == main.after
