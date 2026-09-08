; aliased.ld — quatre banques etendues sur UNE fenetre.
;
; `ext_w1<b>` amene la banque etendue b dans w1 [0x4000..0x7FFF] a la place de
; base1. Quatre configurations, quatre banques, une seule adresse logique : les
; quatre sections `gfx` sont basees a 0x4000 et ne se marchent pas dessus,
; parce qu'elles ne sont jamais presentes ensemble.
;
; `resident` vit en w2, que `ext_w1<b>` ne touche pas : commuter depuis une
; section logee dans w1 la ferait disparaitre sous ses propres pieds.

TARGET cpc6128

MEMORY_MAP {
    CONFIG linear    { w2 { SECTION resident } }
    CONFIG ext_w1<0> { w1 { SECTION gfx0 } }
    CONFIG ext_w1<1> { w1 { SECTION gfx1 } }
    CONFIG ext_w1<2> { w1 { SECTION gfx2 } }
    CONFIG ext_w1<3> { w1 { SECTION gfx3 } }
}
