; expect-error: unrecognized LD form
; §11 One-to-many — `ld hl,ix` does not exist: the DD prefix makes `h` the half
; of IX, so no instruction names H and IXH at once.
    org #4000
    ld hl,ix
