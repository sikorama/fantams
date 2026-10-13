; expect-error: expected relative target
; §4 Expressions, opcode() — written bare, 'e' is the register E, not the
; placeholder: use n, nn, imm, imm8 or imm16 in a bare position.
        db opcode("jr e")
