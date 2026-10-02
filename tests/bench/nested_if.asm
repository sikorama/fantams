; Repeats imbriqués et if par tour : le cas qui a motivé l'ADR 0034
; (4 à 5 s en WASM, 1 s en natif, le 2026-10-02).
    org 0x4000
    repeat 8,j
        repeat #400,x
            ii=(j+(x>>1))&7
            w=0
            if (ii==0)
                db w|%0001,w|%0000
            endif

            if (ii==1)
                db w|%0011,w|%1000
            endif

            if (ii==2)
                db w|%0111,w|%1100
            endif

            if (ii==3)
                db w|%1111,w|%1110
            endif

            if (ii==4)
                db w|%1111,w|%1110
            endif

            if (ii==5)
                db w|%0111,w|%1100
            endif

            if (ii==6)
                db w|%0011,w|%1000
            endif

            if (ii==7)
                db w|%0001,w|%0000
            endif



; db (3<<(i-1))| (x>>2)
; db (3<<(8-i))
        rend
    rend
