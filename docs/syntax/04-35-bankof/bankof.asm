; §4 Expressions, relocatable values — bankof(label) is the storage slot the
; linker placed its section in; bankof(label + 3) is bankof(label).
        section main, "ro"
        db   bankof(sample), bankof(sample + 3), bankof(start)
start:
        nop
        section audio, "ro"
sample:
        nop
