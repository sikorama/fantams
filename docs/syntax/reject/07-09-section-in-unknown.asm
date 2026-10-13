; args: --target cpc6128
; expect-error: which the profile does not declare
; §7.Sections — a configuration the profile does not declare is refused at link.
        section gfx1, "ro" IN nope
        db 1
