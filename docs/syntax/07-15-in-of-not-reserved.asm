; args: --target cpc6128
; §7.Sections — IN and OF are keywords only among a SECTION's operands: IN is
; still the Z80 instruction, and OF can name a label.
        section code, "ro" IN w2 OF linear
OF:     in a,(c)
        in a,(#FE)
