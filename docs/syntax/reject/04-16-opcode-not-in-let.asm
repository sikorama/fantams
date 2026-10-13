; expect-error: opcode() is not available in this context
; §4 Expressions, opcode() — not available while a preprocessor variable is resolved.
        let x = opcode("nop")
