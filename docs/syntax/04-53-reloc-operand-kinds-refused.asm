; expect-error: this operand cannot be a relocatable address
; §4 Expressions, names the linker owns — an index displacement, a bit number,
; an rst vector and an im mode are never addresses.
        section code, "ro"
here:
        ld   a, (ix + here)
