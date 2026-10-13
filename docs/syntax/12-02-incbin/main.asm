; §12 Inclusion — `incbin "file"[, offset[, length]]`; offset and length are
; preprocessor expressions. data.bin holds the bytes 0..39.
    org #4000
SKIP equ 4
whole:
    incbin "data.bin"
part:
    incbin "data.bin", SKIP, SKIP+1
tail:
    incbin "data.bin", 38
    assert part - whole == 40
    assert tail - part == 5
    assert $ - tail == 2
