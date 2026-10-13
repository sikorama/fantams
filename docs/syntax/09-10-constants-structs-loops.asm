; expect-sym: init__1.K,const
; expect-sym: init__1.s,label
; expect-sym: repeat__1.lab,label
; expect-sym: repeat__2.lab,label
; §9.Scope — constants and struct instances written in a body are scoped the same way,
; and each iteration of `repeat`, `while` or `for` is an expansion of its own.
        org 0
        STRUCT T
f1      db 0
        ENDSTRUCT
macro init()
K       EQU 5
        STRUCT T s
endmacro
        init()
        repeat 2
lab:    nop
        endrepeat
