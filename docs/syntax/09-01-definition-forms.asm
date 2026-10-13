; expect-warning: 'third MACRO ...' is an inherited spelling; write 'macro third ...'
; §9.Definition — three notations for a definition; four closures.
; `name MACRO p1,p2` is inherited and warns once per macro.
        org 0
macro first p1,p2
        db p1,p2
endmacro
macro second(p1,p2)
        db p1,p2
endm
third MACRO p1,p2
        db p1,p2
mend
macro fourth()
        db 7
end
start:  first(1,2)
        second(3,4)
        third(5,6)
        third(5,6)
        fourth()
        assert $ - start == 2+2+2+2+1
