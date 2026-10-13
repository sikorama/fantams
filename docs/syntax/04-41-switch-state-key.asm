; args: --target cpc6128
; §4 Expressions, names the linker owns — keyed by a state name from the
; profile, the switching values need no script.
        assert __port_ram_linear == #7F00
        assert __val_linear == #C0
        assert __val_all_ext == #C2
