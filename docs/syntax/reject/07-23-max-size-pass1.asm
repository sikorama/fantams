; expect-error: SECTION 'tab': maximum size is not resolvable in pass 1
; §7.A declared maximum size — the maximum must be known in pass 1: a forward
; equ is refused.
        section tab, "ro", SIZE
        org #4000
        db 1
SIZE    equ 4
