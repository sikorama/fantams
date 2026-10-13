; expect-error: REPEAT without ENDREPEAT or end
; §10.Blocks — `endr` does not exist: it does not close the `repeat`.
        repeat 2
        nop
        endr
