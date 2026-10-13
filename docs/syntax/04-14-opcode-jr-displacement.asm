; expect-error: byte 1 of 'jr nn' is not fixed
; §4 Expressions, opcode() — a jr/djnz displacement is never available: only the
; fixed first byte is.
        assert opcode("jr nn") == #18
        db opcode("jr nn", 1)
