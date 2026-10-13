; args: --target cpc6128
; expect-error: value 0x7F00 does not fit in one byte (-128..255): use high() or low()
; §4 Expressions, composing a port and a value — a byte that is not a byte is
; refused: every eight-bit immediate is bounded to -128..255.
        ld   b, __port_ram_linear
