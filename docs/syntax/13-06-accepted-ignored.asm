; §13 Recognized to be refused — `BUILDSNA`, `BANKSET`, `NOLIST` and `LIST` are
; accepted and ignored: they emit nothing.
    org #4000
    buildsna
    bankset 0
    nolist
    list
    assert $ == #4000
