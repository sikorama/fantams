;--- aliased_org.asm ------------------------------------------------------
; LE MEME PROGRAMME QUE `aliased.asm`, PLACE A LA MAIN.
;
; Ici la source dit tout : la banque de chaque bloc par `org bN:adresse`
; (ADR 0005), et les valeurs de commutation par des `equ` ecrits a la main.
; C'est la forme que prend une source venue d'un autre assembleur, et c'est
; aussi celle qu'il faut quand l'hote ne sait pas editer de script de lien.
;
; Elle n'est pas un pis-aller : un placement peut etre une propriete du
; PROGRAMME. Mais il se paie — chaque banque est nommee DEUX FOIS, une fois
; dans l'`org bN:` et une fois dans l'`equ`, et rien ne garantit qu'elles
; restent d'accord. Dans `aliased.asm`, le linker derive les deux du profil.
;
; `tests/accept_aliased.sh` affirme que les deux fichiers rendent les memes
; octets : c'est ce qui fait de cette redondance un fait verifiable et non une
; affirmation.

; Les valeurs que le profil cpc6128 calcule : OUT 0x7F00, %11000000 | CODE,
; avec CODE = %100 | b pour ext_w1<b>. Ecrites a la main, donc a maintenir.
c_linear equ 0x7FC0        ; = __port_ram_resident | __val_resident
c_ext0   equ 0x7FC4        ; = __port_ram_gfx0     | __val_gfx0
c_ext1   equ 0x7FC5
c_ext2   equ 0x7FC6
c_ext3   equ 0x7FC7

        org    0x8000
        run    start
start:
        ld     de, 0xC000
        ld     bc, c_ext0
        call   read_one
        ld     bc, c_ext1
        call   read_one
        ld     bc, c_ext2
        call   read_one
        ld     bc, c_ext3
        call   read_one
        ld     bc, c_linear
        out    (c), c
stop:   jr     stop

read_one:
        out    (c), c
        ld     a, (0x4000)
        ld     (de), a
        inc    de
        ret

;--- les quatre banques, nommees par leur numero de stockage -------------
; b4..b7 sont ext0..ext3 : le profil declare « BANK ext0..ext3 STORE 4..7 ».
        org    b4:0x4000
gfx0_data:
        db     0xA0, 0x00, 0x01, 0x02
        org    b5:0x4000
gfx1_data:
        db     0xA1, 0x10, 0x11, 0x12
        org    b6:0x4000
gfx2_data:
        db     0xA2, 0x20, 0x21, 0x22
        org    b7:0x4000
gfx3_data:
        db     0xA3, 0x30, 0x31, 0x32
