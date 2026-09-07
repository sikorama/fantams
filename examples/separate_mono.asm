; separate_mono.asm — le MÊME programme que separate_a + separate_b, écrit d'un
; seul tenant. Il n'a ni `public` ni `extern` : dans une seule unité, tout est
; visible, ce qui est exactement la raison d'être de la portée locale par défaut.
;
; Les sections sont déclarées dans le même ORDRE que dans les deux fichiers
; séparés — main, text, puis draw_code, pal — parce que c'est cet ordre que le
; linker suit pour poser ce que personne n'a placé.

    section main, "ro"
entry:
    ld hl, message
    call draw
    ld a, high(palette)
    ld b, low(palette)
    jr draw
countdown:
    djnz countdown
    ret

    section text, "rw"
message:
    db "HI", 0

    section draw_code, "ro"
draw:
    ld a, (hl)
    or a
    ret z
    inc hl
    jr draw
    ret

    section pal, "rw"
palette:
    db 0, 1, 2, 3
