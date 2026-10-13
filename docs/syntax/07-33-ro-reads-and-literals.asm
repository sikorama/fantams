; §7.Writes into "ro" — reads are untouched, and so are a hard-coded address and
; a write through a computed HL: only symbols written in the clear are checked.
        section tables_data, "ro"
        org #4000
my_table:
        db 1, 2, 3, 4
        section execution, "ro"
        org #8000
        ld a, (my_table)
        ld hl, (my_table)
        ld (#4000), a
        ld hl, my_table
        ld (hl), a
