; §4 Expressions, scope between objects — constants in a common header guarded
; by ifndef, included by both sides; the extern lines in a separate file,
; included by the users only.
        include "consts.inc"
        include "consts.inc"            ; twice is harmless
        section libcode, "ro"
        module lib
        public clear
clear:
        ld   hl, SCREEN
        ret
        endmodule
