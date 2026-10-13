; args: --target cpc6128
; expect-error: ORG with a bank prefix inside section 'gfx1'
; §7.Sections — IN already said where the section goes; a bank prefix would
; place it twice.
        section gfx1, "ro" IN w1 OF ext_w1<1>
        org b5:#4000
        db 1
