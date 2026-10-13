; expect-error: "ld (nn), a" writes into read-only section 'tables_data'
; §7.Sections — a section classifies its bytes: a write to a "ro" section's
; label is refused.
        section tables_data, "ro"
        org #4000
my_table:
        db 1, 2, 3, 4

        section execution, "ro"
        org #8000
        ld a, 5
        ld (my_table), a
