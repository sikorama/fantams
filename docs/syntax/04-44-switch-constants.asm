; args: --target cpc6128
; §4 Expressions, names the linker owns — with a profile, these names are plain
; constants: arithmetic on them is ordinary arithmetic.
        org  #4000
first:
        ld   bc, __port_ram_linear | __val_linear   ; one number, computed here
        out  (c), c
        ld   c, __val_linear                        ; and it fits in a byte
last:
        assert (__port_ram_linear | __val_linear) == #7FC0
        assert last - first == 7
