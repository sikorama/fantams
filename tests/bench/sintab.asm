; Une table calculée longue : une seule boucle, une expression flottante par
; ligne déroulée. Mesure le coût par ligne et l'évaluation (ADR 0034).
    org 0x0000
sintab:
    for ii = 0 until 65536
        db 24+20*sin(2*3.14159*ii/256)
    endfor
