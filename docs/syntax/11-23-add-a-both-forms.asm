; args: --strict
; §11 What is refused — `add a,b` and `add b` are both accepted, even by
; `--strict`: one opcode, two standard spellings, no canon elected.
    org #4000
f1:
    add a,b
f2:
    add b
    and a,b
    or a,b
    xor a,b
    assert f2 - f1 == 1
    assert opcode("add a,b") == opcode("add b")
    assert $ - f2 == 4
