; §4 Expressions, opcode() — parenthesizing the whole operand makes it
; 'ld a,(nn)': three bytes reading address 0x0080, not the immediate 0x80.
        org #4000
first:
        ld   a,(opcode("scf") xor opcode("or a"))   ; 3A 80 00
last:
        assert last - first == 3
