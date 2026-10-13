; expect-error: BOUNDARY without a matching END_BOUNDARY
; §7.Boundary blocks — a missing `end_boundary` is refused.
        org #100
        boundary 256
        nop
