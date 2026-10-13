; expect-error: are not in one contiguous block: an org or a section lies between them
; §4 Expressions, nops_between() — an org (or a section change) between the two
; labels is refused.
        org #4000
first:
        nop
        org #5000
second:
        assert tstates_between(first, second) > 0
