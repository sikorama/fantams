; args: --target cpc6128
; §4 Expressions, composing a port and a value — compose with '|', take a byte
; with high() / low(): 3 bytes, or 4.
        org  #4000
one:
        ld   bc, __port_ram_linear | __val_linear
two:
        ld   b, high(__port_ram_linear)
        ld   c, low(__val_linear)
three:
        out  (c), c
        assert two - one == 3 && three - two == 4
        assert high(__port_ram_linear) == #7F
