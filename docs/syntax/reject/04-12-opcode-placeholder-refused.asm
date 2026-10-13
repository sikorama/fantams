; expect-error: 'n' is a placeholder, but this operand is embedded in the opcode byte itself
; §4 Expressions, opcode() — a placeholder where the opcode byte needs a real
; number (bit, rst, im) is refused.
        db opcode("bit n,(hl)", 1)
