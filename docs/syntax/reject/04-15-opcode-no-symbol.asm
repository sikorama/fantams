; expect-error: opcode() resolves no symbol
; §4 Expressions, opcode() — opcode() never resolves a label or a symbol.
target:
        db opcode("jp target")
