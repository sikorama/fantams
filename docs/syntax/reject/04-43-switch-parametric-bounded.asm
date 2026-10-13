; args: --target cpc6128
; expect-error: unknown symbol '__val_ext_w1_4'
; §4 Expressions, names the linker owns — a parametric state is enumerated over
; the values its declared banks bound: BANK ext0..ext3 gives four, no fifth.
        ld   a, __val_ext_w1_4
