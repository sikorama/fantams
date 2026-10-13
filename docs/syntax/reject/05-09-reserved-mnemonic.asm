; expect-error: 'ld' is a Z80 mnemonic and cannot name a macro parameter
; §5.Reserved words — mnemonics and directives are reserved as well.
macro put(ld)
        db ld
endmacro
