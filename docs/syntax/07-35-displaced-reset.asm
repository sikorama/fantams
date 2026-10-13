; expect-sym: here,label,-,0x200,0,0x200,
; §7.Displaced blocks — the displacement is not persistent: a one-address `org`
; ends it.
        org #A600,#100
        db 1
        org #200
here:   db 2
