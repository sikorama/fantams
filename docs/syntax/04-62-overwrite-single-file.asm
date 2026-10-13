; expect-warning: &0100
; §4 Expressions, separate assembly — inside a single file, overwriting an
; address stays a warning: it is an idiom, and the author sees both lines.
        org  #100
        db   1
        org  #100
        db   2
