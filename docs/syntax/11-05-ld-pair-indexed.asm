; args: -E
; expect-out: ld h,(ix+3)
; expect-out: ld l,(ix+2)
; expect-out: ld (iy+0),d
; expect-out: ld (iy-1),e
; §11 One-to-many — `ld rr,(ix+d)` and `ld (ix+d),rr`: the low byte at the low
; address, so the high half takes d+1. High half first.
    org #4000
start:
    ld hl,(ix+2)
    ld (iy-1),de
    assert $ - start == 12
