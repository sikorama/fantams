; args: --target cpc6128
; expect-error: which maps several windows (w0, w1, w2, w3): name the one you mean
; §7.Sections — on cpc6128 `ext_w1<1>` describes the whole map: the short form
; is refused, listing the windows, rather than picking one.
        section gfx1, "ro" IN ext_w1<1>
        db 1
