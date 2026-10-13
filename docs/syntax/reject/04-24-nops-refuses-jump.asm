; expect-error: has no fixed duration: it can transfer control
; §4 Expressions, nops_between() — exact or refused: a jump's duration depends on
; the path taken, and the refusal names the first offending line.
        org #4000
first:
        nop
        jr   $
second:
        assert tstates_between(first, second) > 0
