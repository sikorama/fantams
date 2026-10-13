; §10.ifdef and ifndef — `ifdef` tests whether a name is known, not its value;
; `if` tests the value.
seen = 0
LET DEBUG = 0
        ifdef DEBUG
seen = seen + 1         ; assembled: DEBUG exists, although its value is 0
        endif
        if DEBUG
seen = 100              ; not assembled: the value is 0
        endif
        ifndef NOWHERE
seen = seen + 1
        endif
        assert seen == 2
