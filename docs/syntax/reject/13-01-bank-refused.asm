; expect-error: BANK is not supported: write 'org b4:<address>' instead
; §13 Recognized to be refused — `BANK` names its replacement: bank and address
; go on the same `org` line.
    org #4000
    bank 4
