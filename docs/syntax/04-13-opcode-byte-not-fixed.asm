; expect-error: byte 1 of 'ld a,n' is not fixed
; §4 Expressions, opcode() — asking for a byte that is not fixed — the
; placeholder's own byte — is refused.
        db opcode("ld a,n", 1)
