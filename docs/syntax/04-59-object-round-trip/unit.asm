; §4 Expressions, separate assembly — a .fo is text: writing it, reading it back
; and writing it again gives the same text.
        section code, "ro"
        public entry
entry:
        ld   hl, entry
        ret
