; expect-error: '==' cannot be applied to a relocatable value
; §4 Expressions, names the linker owns — comparisons, boolean operators and the
; real functions are refused on a relocatable value.
        section code, "ro"
here:
        db   here == 3
