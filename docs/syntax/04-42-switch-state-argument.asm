; args: --target cpc6128
; §4 Expressions, names the linker owns — a parametric state is keyed by its
; argument: ext_w1_1 for ext_w1<1>, derived from the profile alone. ext1 comes
; into w1 by ext_w1<1> and by all_ext, with two values: the state names the map.
        assert __val_ext_w1_0 == #C4
        assert __val_ext_w1_1 == #C5
        assert __val_ext_w1_3 == #C7
        assert __val_all_ext == #C2
