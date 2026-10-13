; §4 Expressions, opcode() — a fixed byte of an instruction's encoding; index
; picks the first byte, len how many; a negative len reverses which end weighs most.
        ld   a, opcode("ld (bc),a", 0)          ; 0x02
        assert opcode("ld (bc),a") == #02
        assert opcode("bit 3,(hl)", 1) == #5E   ; the bit number is real
        assert opcode("bit 3,(hl)", 0, 2) == #CB5E
        assert opcode("bit 3,(hl)", 0, -2) == #5ECB
