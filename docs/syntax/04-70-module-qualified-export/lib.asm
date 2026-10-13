; §4 Expressions, scope between objects — inside a module, exported names are
; qualified: 'public init' in MODULE lib exports lib.init.
        section libcode, "ro"
        module lib
        public init
init:
        ret
        endmodule
