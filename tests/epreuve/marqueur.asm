; marqueur.asm — le cas de reference de l'epreuve du snapshot.
;
; ECRIT pour cette epreuve, jamais extrait du corpus (ADR 0029). Minimal et
; autonome : aucun appel firmware, aucune interruption, aucune dependance a
; l'etat de la machine avant le chargement du snapshot.
;
; Ce qu'il fait, et pourquoi : il pose une VALEUR CONNUE a une ADRESSE CONNUE,
; puis boucle. L'epreuve relit cette adresse. Les octets du snapshot, eux,
; portent des zeros a cet endroit — donc lire la valeur, c'est constater que la
; machine a ACCEPTE l'artefact et l'a EXECUTE. Un emulateur qui aurait charge le
; snapshot sans le faire tourner rendrait encore des zeros.
;
; L'epreuve n'affirme rien sur les octets produits par fantams : ceux-la se
; testent depuis une image fabriquee a la main, sans machine.

        org 0x8000
        run start

MARQUEUR equ 0x9000     ; hors du code, en RAM centrale, loin des ROM

start:
        di                      ; aucune interruption : l'etat observe est le notre
        ld sp,0xbf00            ; une pile a nous, dans la RAM du snapshot
        ld a,0x5a
        ld (MARQUEUR),a
        ld hl,0xcafe
        ld (MARQUEUR+1),hl
boucle:
        jr boucle               ; la machine tourne, et l'etat n'evolue plus
