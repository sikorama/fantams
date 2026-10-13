; args: --beautify
; expect-out: start:
; expect-out:         nop
; expect-out:     wait()
; §15 Modes — `--beautify` formats only: label detached on its own line, block
; bodies indented, parentheses added to a known macro.
    org #4000
macro wait()
nop
endmacro
start: ld a,1
    repeat 2
nop
    endrepeat
    wait
