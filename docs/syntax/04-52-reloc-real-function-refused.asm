; expect-error: 'abs()' cannot be applied to a relocatable value
; §4 Expressions, names the linker owns — a real function (sin, abs, min, ...)
; is refused on a relocatable value.
        section code, "ro"
here:
        dw   abs(here)
