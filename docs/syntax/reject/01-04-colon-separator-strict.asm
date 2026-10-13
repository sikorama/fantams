; args: --strict
; expect-error: strict: several instructions on one line
; §1 The line — '--strict' refuses the colon as an instruction separator.
        ld a,1:inc a
