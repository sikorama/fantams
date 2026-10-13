; expect-warning: &8000
; §7.Banks — the offset in the bank is `address & #3FFF`: b4:#4000 and b4:#8000
; land on the same byte, and the overlap is reported.
        org b4:#4000
        db 1
        org b4:#8000
        db 2
