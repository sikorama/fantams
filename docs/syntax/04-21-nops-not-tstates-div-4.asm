; args: --target cpc6128
; §4 Expressions, nops_between() — the NOP count is not the T-states divided by
; 4: push is 11 T-states and 4 NOPs, out (c),r is 12 T-states and 4 NOPs.
        org #4000
p1:
        push hl
p2:
        out  (c), a
p3:
        assert tstates_between(p1, p2) == 11 && nops_between(p1, p2) == 4
        assert tstates_between(p2, p3) == 12 && nops_between(p2, p3) == 4
