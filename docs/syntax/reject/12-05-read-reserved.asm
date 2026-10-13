; expect-error: 'read' is a preprocessor keyword
; expect-error: reserved words are reserved
; §12 Inclusion — `read` is reserved but not implemented: the message is about
; a reserved word, not about a lack.
    org #4000
    read "file.asm"
