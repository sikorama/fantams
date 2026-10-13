; §4 Expressions, names the linker owns — __off_<section> depends on where the
; section landed: it is resolved at link time, and declared with EXTERN.
        section main, "ro"
        extern __off_audio
        ld   hl, __off_audio
        section audio, "ro"
        nop
