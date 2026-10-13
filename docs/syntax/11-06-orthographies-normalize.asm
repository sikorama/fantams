; args: --normalize
; expect-out: jp (hl)
; expect-out: jp (ix)
; expect-out: ex de,hl
; expect-out: ex (sp),hl
; expect-out: ex (sp),iy
; expect-out: db 1
; expect-out: dw 2
; expect-out: ds 3
; expect-out: endmacro
; expect-out: endrepeat
; §11 One-for-one — orthographies are tolerated, and `--normalize` rewrites each
; to its canon.
    org #4000
macro two()
    nop
endm
    ld pc,hl
    jp ix
    ex hl,de
    ex hl,(sp)
    ex iy,(sp)
    defb 1
    defw 2
    rmb 3
    repeat 2
    nop
    rend
