; expect-error: '|' cannot be applied to a relocatable value
; §4 Expressions, names the linker owns — without a profile, an EXTERN switching
; name is relocatable: no arithmetic in that unit but high() / low().
        section main, "ro"
        extern __port_ram_audio
        extern __val_audio
        ld   bc, __port_ram_audio | __val_audio
