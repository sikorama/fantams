;--- banked.asm -----------------------------------------------------------
; L'exemple du §12.2 de docs/spec-chaine-outils.md, sans sa section
; compressee : l'algorithme de compression n'est pas de cet etage (D9), et son
; enveloppe suffit a ce que l'exemple montre.
;
; CE QUE CE FICHIER NE CONTIENT PAS, et c'est tout son objet :
;
;   - aucun `org`     : la fenetre donne son adresse a chaque section
;   - aucune banque   : la configuration dit ou chaque section vit
;   - aucun nombre de commutation : le linker les calcule depuis le profil
;
; Deplacer `audio` d'une banque a l'autre ne touche pas une ligne d'ici.

        SECTION main, "ro"
        run    start
start:
        call   sysbank_unpack
        call   sysbank_audio_init
loop:   call   sysbank_audio_play
        jr     loop

;--- resident : jamais recouvert par une commutation ----------------------
; `sysbank` existe parce que les configurations `ext_w1<b>` donnent `w1` a une
; banque etendue A LA PLACE de `base1`, ou `main` vit. Commuter depuis `main`
; ferait donc disparaitre `main` sous ses propres pieds. Cette contrainte est
; dans les configurations de la machine : l'assembleur ne peut pas la connaitre,
; le linker si — et c'est l'etage C2 qui la refusera.
        SECTION sysbank, "ro"
sysbank_audio_init:
        ld     bc, __port_ram_audio | __val_ram_audio   ; port ET valeur : §12.3
        out    (c), c
        call   audio_init          ; vaut 0x4000 + offset : le linker le sait
        ld     bc, __port_ram_linear | __val_ram_linear
        out    (c), c
        ret

sysbank_audio_play:
        ld     bc, __port_ram_audio | __val_ram_audio
        out    (c), c
        call   audio_play
        ld     bc, __port_ram_linear | __val_ram_linear
        out    (c), c
        ret

sysbank_unpack:
        ld     hl, packed          ; l'enveloppe du §8, non compressee ici
        ld     de, unpacked        ; 0xC000
        ld     bc, packed_size
        call   depack
        ret

depack:
        ldir
        ret

;--- le player, dans la banque etendue -----------------------------------
        SECTION audio, "ro"
audio_init:
        xor    a
        ret
audio_play:
        inc    a
        ret

;--- les donnees, avec un PLAFOND et non une taille ----------------------
; « Tout ce qui suit a une adresse connue, le linker verifie que le compresse
; rentre et signale le mou. » C'est le repli du §8, utilisable des l'etage A.
        SECTION packed, "ro", 0x40
packed:
        db     1, 2, 3, 4, 5, 6, 7, 8
packed_size equ $ - packed

;--- la destination du depacking, aucun octet emis -----------------------
        SECTION unpacked, "uninit"
unpacked:
        ds     0x100
