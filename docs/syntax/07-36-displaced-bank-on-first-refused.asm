; expect-error: ORG: the bank prefix qualifies the STORAGE address, which is the last parameter — write 'org #4000,b4:#100'
; §7.Displaced blocks — the bank prefix goes on the storage (last) parameter.
        org b4:#4000,#100
        db 1
