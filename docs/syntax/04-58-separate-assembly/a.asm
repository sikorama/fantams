; §4 Expressions, separate assembly — '-o x.fo' assembles only; '.fo' inputs are
; linked. Two units linked give the same bytes as the one-file program.
        section code, "ro"
        public start
        extern helper
start:
        call helper
        ret
