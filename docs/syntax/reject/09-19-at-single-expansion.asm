; expect-error: duplicate symbol: 'entry' — the macro 'boot' marks it with '@'
; §9.Scope — a macro with an `@` label can be expanded only once; the error falls
; on the second call.
macro boot()
@entry: nop
endmacro
        boot()
        jp entry
        boot()
