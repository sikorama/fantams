; §4 Expressions, functions — sizeof gives the size of a struct.
        struct point
x       db 0
y       dw 0
        endstruct
        assert sizeof(point) == 3
