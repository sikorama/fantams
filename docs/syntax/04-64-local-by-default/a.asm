; §4 Expressions, scope between objects — a symbol is local to its object by
; default: two files use the same internal label without colliding.
        section c1, "ro"
loop:
        djnz loop
