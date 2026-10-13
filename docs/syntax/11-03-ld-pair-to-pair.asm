; args: -E
; expect-out: ld d,h
; expect-out: ld e,l
; expect-out: ld b,hy
; expect-out: ld c,ly
; expect-out: ld hx,d
; expect-out: ld lx,e
; §11 One-to-many — `ld rr,rr'` across BC, DE, HL, IX, IY, both directions,
; through the undocumented index halves.
    org #4000
start:
    ld de,hl
    ld bc,iy
    ld ix,de
    assert $ - start == 10
