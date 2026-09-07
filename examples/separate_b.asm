; separate_b.asm — la moitié « bibliothèque » de l'exemple à deux unités.
; Voir separate_a.asm pour le mode d'emploi et ce qui est prouvé.

    public draw
    public palette
    extern entry

    section draw_code, "ro"
draw:
    ld a, (hl)
    or a
    ret z
    inc hl
    jr draw                 ; intra-section : mesuré à l'assemblage
    ret

    section pal, "rw"
palette:
    db 0, 1, 2, 3
