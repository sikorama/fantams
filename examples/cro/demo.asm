; demo.asm — une ROM d'arriere-plan minimale : l'en-tete que le firmware lit
; au demarrage (type, version, table des noms, vecteurs), et une init qui ne
; reserve rien. Le slot n'est ecrit NULLE PART ici (ADR 0004) : c'est le
; script -T qui la place, et `--cro-rom` qui le redit au conteneur.
        section rom, "ro"

        db   1                  ; ROM d'arriere-plan
        db   1, 0, 0            ; marque, version, modification
        dw   names
        jp   init

names:  db   "DEM", 'O' | 0x80
        db   0

init:   ret
