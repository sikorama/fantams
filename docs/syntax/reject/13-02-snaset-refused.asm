; expect-error: snaset describes the OUTPUT format, not the program
; expect-error: setcpc describes the OUTPUT format, not the program
; §13 Recognized to be refused — `SNASET` and `SETCPC` describe the output, which
; the invocation chooses.
    org #4000
    snaset
    setcpc 0
