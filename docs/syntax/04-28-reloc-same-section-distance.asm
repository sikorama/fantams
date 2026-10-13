; §4 Expressions, relocatable values — end - start in the same section is a plain
; number: this is how a table is measured.
        section data, "ro"
table:
        db   1, 2, 3
table_end:
        assert table_end - table == 3
        db   table_end - table
