; expect-error: there is no conditional RST
; §11 What is refused — `rst z,#38` would be 2 overlapping bytes, which no pair
; of canonical Z80 lines expresses.
    org #4000
    rst z,#38
