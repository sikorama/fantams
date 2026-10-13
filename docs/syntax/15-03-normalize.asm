; args: --normalize
; expect-out: macro save()
; expect-out: push hl
; expect-out: push de
; expect-out: nop 4
; expect-out: save()
; §15 Modes — `--normalize` canonicalizes without unrolling: the macro stays a
; macro, the count stays a count, `push hl,de` becomes two lines.
    org #4000
macro save()
    push hl,de
endmacro
    save()
    nop 4
