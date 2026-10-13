; expect-error: CHARSET is not supported: a character-set permutation is an asset encoding
; §13 Recognized to be refused — `CHARSET`: generate the `db` with a script.
    org #4000
    charset "abc",0
