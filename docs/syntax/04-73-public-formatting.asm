; args: --beautify
; expect-out:     public entry
; expect-out: entry:
; §4 Expressions, scope between objects — formatting knows them: a 'public' at
; the start of a line is a directive, never a label to be given a colon.
public entry
entry
        ret
