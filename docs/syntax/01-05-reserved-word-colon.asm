; expect-warning: 'nop:' is read as two statements, not as a label
; §1 The line — a reserved word before a colon is never a label: 'nop:nop:nop'
; is three nops. It is flagged all the same, because 'nop:' reads like a label.
        org #4000
first:
        nop:nop:nop
last:
        assert last - first == 3
