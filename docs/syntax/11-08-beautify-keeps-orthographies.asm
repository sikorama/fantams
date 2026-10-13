; args: --beautify
; expect-out: jp hl
; expect-out: defb 1
; expect-out: ex hl,de
; §11 One-for-one — `--beautify` formats; it does not canonicalize.
    org #4000
    jp hl
    defb 1
    ex hl,de
