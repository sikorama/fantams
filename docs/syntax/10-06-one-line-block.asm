; §10.Blocks — a block can open and close on one line.
        org 0
start:  repeat 3 : dw 1,2 : rend
        assert $ - start == 12
