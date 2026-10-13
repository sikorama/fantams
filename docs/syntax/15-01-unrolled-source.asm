; args: -E
; expect-out: db 0*2
; expect-out: db 1*2
; expect-out: push hl
; expect-out: push de
; §15 Modes — `-E` writes the unrolled source: macros expanded, loops unrolled,
; canonicalized. It still assembles, so the asserts run.
    org #4000
macro save()
    push hl,de
endmacro
start:
    for k = 0 until 2
    db k*2
    endfor
    save()
    assert $ - start == 4
