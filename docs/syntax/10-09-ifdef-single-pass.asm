; §10.ifdef and ifndef — one pass: a name defined below the test is not known yet.
seen = 0
        ifdef later
seen = 100
        endif
later:
        ifdef later
seen = seen + 1
        endif
        assert seen == 1
