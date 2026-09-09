;--- aliased_sym.asm ------------------------------------------------------
; LE MEME PROGRAMME QUE `aliased.asm` ET `aliased_org.asm`, TROISIEME FACON.
;
; Ici la source place — comme `aliased_org.asm` — mais elle ne CALCULE rien :
; chaque section dit dans quelle configuration du profil elle vit, et le linker
; en tire la banque, l'adresse logique ET la valeur de commutation.
;
; C'est ce qui manquait a `aliased_org.asm`, dont l'en-tete nomme lui-meme le
; prix qu'il paie : « chaque banque est nommee DEUX FOIS, une fois dans l'`org
; bN:` et une fois dans l'`equ`, et rien ne garantit qu'elles restent
; d'accord ». Ici elle est nommee une fois. Il n'y a plus un seul `equ`, plus un
; seul `org`, et plus un seul nombre de commutation ecrit a la main.
;
; Ce que ce fichier paie a son tour, et qu'il faut savoir : il est COUPLE A LA
; MACHINE. `ext_w1<1>` est un nom du profil cpc6128 ; ce source ne se construit
; pas pour une autre cible sans etre edite, la ou `aliased.asm` change de
; machine en changeant de script. C'est un echange, et il est deliberé —
; l'ADR 0030 le dit en entier.
;
; `tests/accept_aliased.sh` affirme que les TROIS fichiers rendent les memes
; octets. Aucun oracle exterieur : trois chemins du meme outil, et leur accord
; fait la preuve.
;
; LA FORME VERBEUSE, et pourquoi. `IN <config>` suffirait si la configuration ne
; mappait qu'une fenetre. Sur cpc6128 un etat decrit TOUTE la carte —
; `ext_w1<b>` s'ecrit `{ w0 base0  w1 ext<b>  w2 base2  w3 base3 }` —, donc il
; faut nommer la fenetre. Le linker refuse la forme courte en listant les
; quatre, plutot que d'en choisir une.
;
; La contrainte que l'exemple respecte : on ne commute pas depuis une section
; qui vit dans la fenetre commutee. `resident` est en w2, jamais recouvert.

        SECTION resident, "ro" IN w2 OF linear
        run    start

; Lire le premier octet de chacune des quatre banques, dans l'ordre, et les
; deposer en 0xC000. Chaque banque porte une marque differente : si le
; placement changeait, l'ordre des octets lus changerait — et rien d'autre.
;
; Les symboles employes ici nomment la CONFIGURATION et non la section :
; `__val_ram_ext_w1_0` est « la valeur qui amene ext0 en w1 ». Ils se derivent
; du profil seul, sans script — c'est ce qui les rend disponibles a une source
; qui se place elle-meme.
start:
        ld     de, 0xC000
        ld     bc, __port_ram_ext_w1_0 + __val_ram_ext_w1_0
        call   read_one
        ld     bc, __port_ram_ext_w1_1 + __val_ram_ext_w1_1
        call   read_one
        ld     bc, __port_ram_ext_w1_2 + __val_ram_ext_w1_2
        call   read_one
        ld     bc, __port_ram_ext_w1_3 + __val_ram_ext_w1_3
        call   read_one
        ld     bc, __port_ram_linear + __val_ram_linear      ; on rend la fenetre
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

;--- les quatre banques, chacune dans sa configuration -------------------
; Aucune ne nomme un numero de banque : `ext_w1<0>` dit « la configuration qui
; amene la premiere banque etendue », et le profil dit que celle-la se range en
; 4. Deplacer une section d'une configuration a l'autre est UNE LIGNE, et le
; linker en tire la nouvelle valeur de commutation tout seul.
        SECTION gfx0, "ro" IN w1 OF ext_w1<0>
gfx0_data:
        db     0xA0, 0x00, 0x01, 0x02
        SECTION gfx1, "ro" IN w1 OF ext_w1<1>
gfx1_data:
        db     0xA1, 0x10, 0x11, 0x12
        SECTION gfx2, "ro" IN w1 OF ext_w1<2>
gfx2_data:
        db     0xA2, 0x20, 0x21, 0x22
        SECTION gfx3, "ro" IN w1 OF ext_w1<3>
gfx3_data:
        db     0xA3, 0x30, 0x31, 0x32
