; Démo fantams : macros, REPEAT, IF, une expansion est un espace de noms + export SNA
LET DEBUG = 1
LET COUNT = 4
        org 0x8000
        run start

; macro avec un label écrit en dur : il est local à chaque expansion (WAIT__1.wloop,
; WAIT__2.wloop), donc deux appels ne se heurtent pas. Pour qu'un label d'une macro
; reste global, on le marque '@' (la macro ne se développe alors qu'une fois) ou on le
; construit par argument ({n}_fin:). Le compteur est par macro : WAIT__1, WAIT__2.
MACRO WAIT(n)
wloop:  dec {n}
        jr nz,wloop
ENDM

start:
        ld b,{COUNT}    ; {} = substitution PP (COUNT est une variable préprocesseur)
        WAIT(b)         ; 1er appel -> WAIT__1.wloop
        WAIT(b)         ; 2e appel  -> WAIT__2.wloop (pas de collision)

; table générée par FOR : les bornes sont écrites, rien à deviner.
; `to` inclut la borne haute, `until` l'exclut.
tbl:
FOR k = 1 TO COUNT
        db {=k*k}       ; carrés : 1,4,9,16
END

IF DEBUG
        ld a,0xFF       ; inclus seulement si DEBUG
ELSE
        ld a,0
ENDIF
