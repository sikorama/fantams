; expect-error: BOUNDARY inside another BOUNDARY block is not supported
; §7.Boundary blocks — nesting is refused rather than mis-measured.
        org #100
        boundary 256
        boundary 16
        nop
        end_boundary
        end_boundary
