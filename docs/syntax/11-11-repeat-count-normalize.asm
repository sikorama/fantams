; args: --normalize
; expect-out: nop 32
; §11 Repetition — `--normalize` canonicalizes without unrolling: `nop 32` stays.
    org #4000
    nop 32
