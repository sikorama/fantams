; expect-error: "ld (nn), hl" writes into read-only section 'tables_data'
; §7.Writes into "ro" — the check follows an expression on the symbol.
        section tables_data, "ro"
        org #4000
my_table:
        db 1, 2, 3, 4
        section execution, "ro"
        org #8000
        ld (my_table+1), hl
