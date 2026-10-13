; expect-error: bankof() takes a relocatable address: an absolute one carries no linker decision to report
; §4 Expressions, relocatable values — bankof() of an absolute address is refused.
        db   bankof(#1234)
