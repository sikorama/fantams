; expect-error: assertion failed: table_end - table == 4 — table must stay 4 bytes
; §8 Diagnostics in the source — `assert` fails in pass 2, so it reads labels
; defined below it; the optional message follows the condition.
        org #100
        assert table_end - table == 4, "table must stay 4 bytes"
table:  db 1, 2, 3
table_end:
