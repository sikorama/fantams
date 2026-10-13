; §1 The line — 'nop : nop : nop' says plainly that the colon separates
; instructions, and is not flagged.
        org #4000
first:
        nop : nop : nop
last:
        assert last - first == 3
