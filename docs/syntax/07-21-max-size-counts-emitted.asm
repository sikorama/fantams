; §7.A declared maximum size — the size is the bytes emitted over every
; reopening; align and boundary do not count; exactly the maximum is accepted.
        section tab, "ro", 4
        org #4000
        db 1
        align 256
        boundary 256
        db 2
        end_boundary
        section other, "ro"
        org #5000
        db 0
        section tab, "ro"
        org #6000
        db 3, 4
