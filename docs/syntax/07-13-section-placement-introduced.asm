; args: --target cpc6128
; expect-error: SECTION 'gfx1' was first declared without a placement
; §7.Sections — introducing an IN at a reopening is refused: it would move the
; bytes already in the section.
        section gfx1, "ro"
        db 1
        section gfx1, "ro" IN w1 OF ext_w1<2>
        db 2
