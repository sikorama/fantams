; expect-error: 'low' is an assembler directive and cannot name a symbol
; §4 Expressions, scope between objects — public, extern, high and low are
; reserved at every phase: they cannot name a label or a symbol.
low     equ 3
