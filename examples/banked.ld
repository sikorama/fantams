; banked.ld — la carte memoire de examples/banked.asm.
;
; Dix lignes pour un programme banque : c'est l'affirmation du §6, et
; tests/accept_banked.sh la tient.
;
; Le placement se dit DANS UNE CONFIGURATION, et la fenetre en decoule : c'est
; elle qui donnera son ORG a la section. Ecrire « w1 { SECTION audio } » sous
; « ext_w1<1> » dit d'un seul geste « dans la banque ext1 » et « vue en 0x4000 »,
; sans que la source ait a le savoir.

TARGET cpc6128

MEMORY_MAP {
    CONFIG linear {
        w1 { SECTION main     }
        w2 { SECTION sysbank  }
        w3 { SECTION unpacked }
    }
    CONFIG ext_w1<1> { w1 { SECTION audio  SECTION packed } }
}
