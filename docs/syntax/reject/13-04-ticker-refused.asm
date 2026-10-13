; expect-error: TICKER is not supported: cycle counting is a control-flow analysis
; expect-error: write nops_between(a, b) on two labels instead
; §13 Recognized to be refused — `TICKER` points to `nops_between()`.
    org #4000
    ticker start,count
