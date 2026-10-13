; expect-error: '@' marks a label of a macro or loop body
; §9.Scope — at the top level every label is global already: `@x:` is an error.
@x:     nop
