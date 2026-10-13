; §4 Expressions, opcode() — an operand that becomes a separate byte is written
; as a placeholder (n nn d e imm imm8 imm16, any case); which one does not matter.
        assert opcode("ld a,n") == #3E
        assert opcode("ld a,IMM8") == #3E
        assert opcode("ld hl,nn") == #21
        assert opcode("jp imm16") == #C3
        assert opcode("ld a,(ix+d)", 0, 2) == #DD7E
        assert opcode("ld a,(iy+E)", 1) == #7E
        assert opcode("rst #38") == #FF                 ; a vector is real
        assert opcode("im 1", 1) == #56                 ; so is a mode
