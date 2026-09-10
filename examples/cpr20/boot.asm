        SECTION main, "ro"
        run start

start:
        di
        ld   sp, 0x8000

        ; --- CRTC : les valeurs standard fournies (aucun firmware pour les poser) ---
        ld   bc, 0xBC00 | 0
        out (c), c
        ld bc, 0xBD00 | 0x3F
        out (c), c
        ld   bc, 0xBC00 | 1
        out (c), c
        ld bc, 0xBD00 | 0x28
        out (c), c
        ld   bc, 0xBC00 | 2
        out (c), c
        ld bc, 0xBD00 | 0x2E
        out (c), c
        ld   bc, 0xBC00 | 3
        out (c), c
        ld bc, 0xBD00 | 0x8E
        out (c), c
        ld   bc, 0xBC00 | 4
        out (c), c
        ld bc, 0xBD00 | 0x26
        out (c), c
        ld   bc, 0xBC00 | 5
        out (c), c
        ld bc, 0xBD00 | 0x00
        out (c), c
        ld   bc, 0xBC00 | 6
        out (c), c
        ld bc, 0xBD00 | 0x19
        out (c), c
        ld   bc, 0xBC00 | 7
        out (c), c
        ld bc, 0xBD00 | 0x1E
        out (c), c
        ld   bc, 0xBC00 | 8
        out (c), c
        ld bc, 0xBD00 | 0x00
        out (c), c
        ld   bc, 0xBC00 | 9
        out (c), c
        ld bc, 0xBD00 | 0x07
        out (c), c
        ld   bc, 0xBC00 | 12
        out (c), c
        ld bc, 0xBD00 | 0x30
        out (c), c
        ld   bc, 0xBC00 | 13
        out (c), c
        ld bc, 0xBD00 | 0x00
        out (c), c

        ; --- Palette (mode 1, 4 crayons) : sans firmware, rien ne l'initialise ---
        ld   bc, 0x7F00 | 0
        out (c), c
        ld bc, 0x7F00 | (0x40 | 0)
        out (c), c
        ld   bc, 0x7F00 | 1
        out (c), c
        ld bc, 0x7F00 | (0x40 | 26)
        out (c), c
        ld   bc, 0x7F00 | 2
        out (c), c
        ld bc, 0x7F00 | (0x40 | 6)
        out (c), c
        ld   bc, 0x7F00 | 3
        out (c), c
        ld bc, 0x7F00 | (0x40 | 18)
        out (c), c

        ld   bc, 0x7F89          ; RMR: rom haute off, rom basse on, mode 1
        out  (c), c

loop:
        ld   bc, 0x7F81          ; RMR: rom haute ON (bit3=0), rom basse inchangee
        out  (c), c

        FOR n = 1 TO 20
        ld   bc, 0xDF00 | (0x80 | n)
        out  (c), c

        ld   hl, 0xC000
        ld   de, 0xC000
        ld   bc, 0x4000
        ldir

        call delay
        ENDFOR

        ld   bc, 0x7F89
        out  (c), c
        jp   loop

delay:
        push bc
        ld   b, 2
delay_outer:
        push bc
        ld   de, 0
delay_inner:
        dec  de
        ld   a, d
        or   e
        jr   nz, delay_inner
        pop  bc
        djnz delay_outer
        pop  bc
        ret
