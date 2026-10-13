; args: -E
; expect-out: ld a,1
; expect-out: inc a
; §11 One-to-many — `:` separates statements; the unrolled source shows two lines.
    org #4000
start:
    ld a,1:inc a
    assert $ - start == 3
