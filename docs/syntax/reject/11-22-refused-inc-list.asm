; expect-error: INC takes a single operand: a multi-register list is accepted only on PUSH/POP
; §11 What is refused — `inc hl,de`: register lists stay on `push`/`pop`.
    org #4000
    inc hl,de
