; separate_a.asm — la moitié « programme » d'un exemple en DEUX unités.
;
; Assemblée seule en .fo, puis linkée avec separate_b.fo, elle doit produire le
; MÊME binaire, octet pour octet, que separate_mono.asm — qui est le même
; programme écrit d'un seul tenant. C'est le critère de fin de l'étage B.
;
;   fantams examples/separate_a.asm -o a.fo
;   fantams examples/separate_b.asm -o b.fo
;   fantams a.fo b.fo -o separate.bin
;   fantams examples/separate_mono.asm -o mono.bin
;   cmp separate.bin mono.bin        # identiques
;
; Aucune section ne porte d'org : c'est le LINKER qui les place.

    public entry
    public message
    extern draw
    extern palette

    section main, "ro"
entry:
    ld hl, message          ; Abs16 vers une section de CET objet
    call draw               ; Abs16 vers un symbole de l'AUTRE
    ld a, high(palette)     ; High8 vers l'autre objet
    ld b, low(palette)      ; Low8, le même
    jr draw                 ; saut relatif INTER-sections : le linker mesure
countdown:
    djnz countdown          ; saut relatif intra-section : l'encodeur mesure
    ret

    section text, "rw"
message:
    db "HI", 0
