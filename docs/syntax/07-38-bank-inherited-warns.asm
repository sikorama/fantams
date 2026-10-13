; expect-warning: ORG without a bank prefix inherits bank 4
; §7.Banks — the bank persists from one `org` to the next; inheriting one
; outside the base 64 K warns.
        org b4:#4000
        db 1
        org #4100
        db 2
