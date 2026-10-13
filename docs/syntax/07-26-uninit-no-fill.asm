; expect-error: DS in a "uninit" section reserves space: it cannot take a fill value
; §7."uninit" — `ds` takes no fill value there.
        section vars, "uninit"
        org #C000
        ds 16,#FF
