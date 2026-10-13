; expect-error: STR is not implemented yet: use 'db'
; §13 Recognized to be refused — `STR` is reserved, not yet implemented.
    org #4000
    str "ab"
