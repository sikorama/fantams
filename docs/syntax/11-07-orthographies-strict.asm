; args: --strict
; expect-error: strict: 'jp hl' is a non-canonical spelling — write 'jp (hl)'
; expect-error: strict: 'defb' is a non-canonical spelling — write 'db'
; §11 One-for-one — `--strict` refuses an orthography, naming its canon.
    org #4000
    jp hl
    defb 1
