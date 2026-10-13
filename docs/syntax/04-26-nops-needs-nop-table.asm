; expect-error: this target does not say how long a NOP lasts
; §4 Expressions, nops_between() — needs a target that declares NOP_TABLE;
; without one it says so and points to tstates_between(), which works everywhere.
first:
        nop
second:
        assert tstates_between(first, second) == 4
        assert nops_between(first, second) == 1
