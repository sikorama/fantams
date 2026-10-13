; args: --beautify --no-detach-labels
; expect-out: start: ld a,1
; §15 Modes — `--no-detach-labels` keeps `label: instruction` on one line.
    org #4000
start: ld a,1
