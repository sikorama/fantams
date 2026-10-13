; expect-warning: 'v' has no preprocessor value at the call site
; §9.Arguments — an expression depending on a label falls back to text and warns,
; on the body line, naming the call site; once per (body line, call site).
        org 0
macro w(v)
        dw v
endmacro
        repeat 2
        w(buffer+2)     ; one warning for both iterations
        endrepeat
buffer: db 0
