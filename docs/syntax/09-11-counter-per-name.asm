; expect-sym: toto__1.x,label
; expect-sym: titi__1.x,label
; expect-sym: toto__2.x,label
; expect-sym: titi__2.x,label
; expect-sym: lib.put__1.x,label
; expect-sym: lib.put__2.x,label
; §9.Scope — the call number counts per name; a macro of a module counts under its
; qualified name (`lib.put`), wherever it is called from.
        org 0
macro toto()
x:      nop
endmacro
macro titi()
x:      nop
endmacro
        toto()
        titi()
        toto()
        titi()
        MODULE lib
macro put()
x:      nop
endmacro
        ENDMODULE
        lib.put()
        lib.put()
