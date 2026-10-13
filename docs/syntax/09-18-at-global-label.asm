; expect-sym: entry,label
; expect-sym: K,const
; expect-sym: s,label
; §9.Scope — `@` marks a global label (the `@` is not part of the name), and marks
; constants and struct instances the same way. Such a macro is expanded once.
        org 0
        STRUCT T
f1      db 0
        ENDSTRUCT
macro boot()
@entry: nop
@K      EQU 5
        STRUCT T @s
endmacro
        boot()
        jp entry
        assert K == 5
        assert s.f1 == s
