; expect-error: '5' is a literal value where opcode() expects a placeholder
; §4 Expressions, opcode() — a real value where a placeholder is required is refused.
        db opcode("ld a,5")
