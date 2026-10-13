; §4 Expressions, names the linker owns — keyed by a section name, the switching
; values need a link script: the script binds the section to a configuration.
        section main, "ro"
        ld   bc, __port_ram_audio | __val_audio
        out  (c), c
        section audio, "ro"
        nop
