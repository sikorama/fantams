; §1 The line — comments run from ';' or '//' to the end of the line;
; '/* ... */' can span several lines.
        org #4000
first:
        nop             ; a comment
        nop             // a comment too
        /* this one
           spans two lines */ nop
last:
        assert last - first == 3
