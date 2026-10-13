; args: --target cpc6128
; expect-sym: gfx1_data,label,gfx1,0x4000,5,0x4000
; expect-sym: gfx2_data,label,gfx2,0x4000,6,0x4000
; §7.Sections — a section says where it goes, in the profile's words: a window
; OF a configuration, with a size cap beside it if wanted.
        section gfx1, "ro" IN w1 OF ext_w1<1>
gfx1_data:
        db 1
        section gfx2, "ro", 0x2000 IN w1 OF ext_w1<2>
gfx2_data:
        db 2
