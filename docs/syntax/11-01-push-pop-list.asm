; args: -E
; expect-out: push hl
; expect-out: push de
; expect-out: pop af
; expect-out: pop bc
; §11 One-to-many — `push`/`pop` take a register list, unrolled into one
; instruction per register, in the order written.
    org #4000
start:
    push hl,de
    pop af,bc
    assert $ - start == 4
