; §10.Blocks — `for var = low to high` includes the high bound; `until` excludes it.
incl = 0
excl = 0
        for k = 0 to 3
incl = incl + 1
        endfor
        for k = 0 until 3
excl = excl + 1
        endfor
        assert incl == 4
        assert excl == 3
