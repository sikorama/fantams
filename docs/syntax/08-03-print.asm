; expect-warning: : 3 bytes at 256
; §8 Diagnostics in the source — `print` shows values and strings at assembly,
; with the final value of a label defined below.
        org #100
        print size, " bytes at ", table
table:  db 1, 2, 3
size    equ $ - table
