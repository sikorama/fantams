; expect-error: '__val_mine' starts with '__', which belongs to the linker
; §4 Expressions, names the linker owns — they are reserved: defining one is
; refused, so the program never ships the author's value instead of the placement's.
__val_mine equ #C5
