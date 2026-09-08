;--- aliased.asm ----------------------------------------------------------
; QUATRE BANQUES VUES A LA MEME ADRESSE LOGIQUE.
;
; C'est la forme qu'on rencontre des qu'un programme depasse les 64 K : quatre
; banques etendues qui occupent tour a tour la meme fenetre, et du code
; resident qui commute entre elles. `banked.asm` n'exerce qu'une seule banque
; etendue ; ici elles sont quatre, et elles portent toutes l'adresse 0x4000.
;
; Ce fichier ne dit RIEN de leur placement — ni banque, ni org, ni valeur de
; commutation. Il nomme des sections ; `aliased.ld` dit ou elles vivent, et le
; linker calcule le reste. Son jumeau `aliased_org.asm` fait le contraire : il
; place tout a la main. Les deux doivent rendre les MEMES OCTETS, et
; `tests/accept_aliased.sh` le tient.
;
; La contrainte que l'exemple respecte : on ne commute pas depuis une section
; qui vit dans la fenetre commutee. `resident` est en w2, jamais recouvert.

        SECTION resident, "ro"
        run    start

; Lire le premier octet de chacune des quatre banques, dans l'ordre, et les
; deposer en 0xC000. Chaque banque porte une marque differente : si le
; placement changeait, l'ordre des octets lus changerait — et rien d'autre.
start:
        ld     de, 0xC000
        ld     bc, __port_ram_gfx0 + __val_ram_gfx0
        call   read_one
        ld     bc, __port_ram_gfx1 + __val_ram_gfx1
        call   read_one
        ld     bc, __port_ram_gfx2 + __val_ram_gfx2
        call   read_one
        ld     bc, __port_ram_gfx3 + __val_ram_gfx3
        call   read_one
        ld     bc, __port_ram_resident + __val_ram_resident   ; on rend la fenetre
        out    (c), c
stop:   jr     stop

; bc = le port ET la valeur, en un seul nombre (§12.3). La banque arrive dans
; w1, on lit son premier octet, on l'ecrit en (de).
read_one:
        out    (c), c
        ld     a, (0x4000)
        ld     (de), a
        inc    de
        ret

;--- les quatre banques, toutes basees a 0x4000 --------------------------
        SECTION gfx0, "ro"
gfx0_data:
        db     0xA0, 0x00, 0x01, 0x02
        SECTION gfx1, "ro"
gfx1_data:
        db     0xA1, 0x10, 0x11, 0x12
        SECTION gfx2, "ro"
gfx2_data:
        db     0xA2, 0x20, 0x21, 0x22
        SECTION gfx3, "ro"
gfx3_data:
        db     0xA3, 0x30, 0x31, 0x32
