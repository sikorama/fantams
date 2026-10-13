; §10.ifdef and ifndef — what counts as known: a preprocessor variable, a macro, a
; macro argument or loop index, a constant or variable (computable or not), a label above.
        org 0
known = 0
LET FLAG = 0
        ifdef FLAG
known = known + 1
        endif
macro mac(arg)
        ifdef arg
known = known + 1
        endif
endmacro
        ifdef mac
known = known + 1
        endif
        mac(1)
        for cnt = 0 until 1
        ifdef cnt
known = known + 1
        endif
        endfor
C1      EQU 3
        ifdef C1
known = known + 1
        endif
V1 = 4
        ifdef V1
known = known + 1
        endif
C2      EQU start+1     ; depends on a label: known, not computable yet
        ifdef C2
known = known + 1
        endif
start:  nop
        ifdef start
known = known + 1
        endif
        assert known == 8
