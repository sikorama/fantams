; §7.Boundary blocks — the block is measured (5 bytes here): it stays in place
; when it fits in the current 256-byte page, else it skips to the next one.
label_1 equ 1
label_2 equ 2
        org #2FFE
        boundary 256
moved:  dw label_1
        dw label_2
        db #FF
        end_boundary
        assert moved == #3000

        org #2F00
        boundary 256
kept:   dw label_1
        dw label_2
        db #FF
        end_boundary
        assert kept == #2F00
