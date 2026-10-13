; expect-warning: 'ex af,af' reads literally as exchanging AF with itself, which is a no-op
; §11 One-for-one — `ex af,af` is the only tolerance that warns: its literal
; reading is another operation. It still assembles to `ex af,af'`.
    org #4000
start:
    ex af,af
    assert $ - start == 1
