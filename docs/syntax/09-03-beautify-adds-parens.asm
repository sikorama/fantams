; args: --beautify
; expect-out: pair(3,4)
; §9.Call — `--beautify` adds the parentheses to a macro it knows.
        org 0
macro pair(x1,y1)
        db x1,y1
endmacro
        pair 3,4
