; §4 Expressions, opcode() — opcode() returns a plain number, so it combines with
; any operator; the leading 'xor' is the mnemonic, the inner one the operator.
        org #4000
        assert (opcode("scf") xor opcode("or a")) == #80
first:
        xor  opcode("scf") xor opcode("or a")    ; EE 80
        ld   a, opcode("scf") xor opcode("or a") ; 3E 80
last:
        assert last - first == 4
