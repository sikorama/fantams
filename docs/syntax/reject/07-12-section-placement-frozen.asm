; args: --target cpc6128
; expect-error: a section keeps the placement of its first declaration
; §7.Sections — the placement is frozen at the first declaration: reopening
; without IN keeps it, reopening with another one is refused.
        section gfx1, "ro" IN w1 OF ext_w1<1>
        db 1
        section gfx1, "ro"
        db 2
        section gfx1, "ro" IN w1 OF ext_w1<2>
        db 3
