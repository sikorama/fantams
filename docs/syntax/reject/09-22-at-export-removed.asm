; expect-error: '@@export' no longer exists: use '@' on the label
; §9.Scope — `@@export` no longer exists; it is refused, pointing to `@`.
macro m()
@@export x
x:      nop
endmacro
        m()
