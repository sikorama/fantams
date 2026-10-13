; expect-error: there is no 16-bit load from SP
; expect-error: clobbers the carry
; §11 What is refused — `ld hl,sp` would be 4 bytes and clobber the carry.
    org #4000
    ld hl,sp
