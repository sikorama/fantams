; args: -E
; expect-out:     ld a,1
; expect-out:     inc a
; §1 The line — the colon also separates instructions: 'ld a,1:inc a' is two,
; canonicalized to two lines by the preprocessor (-E shows them).
        org #4000
first:
        ld a,1:inc a
last:
        assert last - first == 3
