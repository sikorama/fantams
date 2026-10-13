; args: --beautify --sym
; expect-error: --sym et --timing demandent un assemblage
; §15 Modes — `--sym` refuses `--beautify` (and `--normalize`): they never reach
; the assembler, so there is no table.
    org #4000
start:
    nop
