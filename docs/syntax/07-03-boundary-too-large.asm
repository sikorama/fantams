; expect-error: Block 'big' (5 bytes) exceeds the boundary limit (4 bytes)
; §7.Boundary blocks — a block larger than its boundary can never fit; the first
; label names it in the diagnostic.
        org #100
        boundary 4
big:    ds 5
        end_boundary
