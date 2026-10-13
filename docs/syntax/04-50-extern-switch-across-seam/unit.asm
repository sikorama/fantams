; §4 Expressions, names the linker owns — a unit assembled without a profile
; declares the names with EXTERN; the linker resolves them, high() included.
        section main, "ro"
        extern __port_ram_audio
        extern __val_audio
        ld   bc, __port_ram_audio       ; the port the profile declares
        ld   b, high(__port_ram_audio)  ; and its high byte, across the seam
        ld   hl, __val_audio            ; the value that switches to audio's bank
        section audio, "ro"
        nop
