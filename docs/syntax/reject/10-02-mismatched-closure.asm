; expect-error: 'ENDWHILE' closes a WHILE block, but the open block here is a REPEAT
; §10.Blocks — a named closure that does not match is an error stating so.
        repeat 2
        nop
        endwhile
