; expect-error: it measures assembled code, which does not exist yet at preprocessor time
; §4 Expressions, nops_between() — it does not exist at preprocessor time:
; 'let', a 'repeat' count and 'nop n' refuse it.
first:
        nop
second:
        let n = tstates_between(first, second)
