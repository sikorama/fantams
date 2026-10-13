; §10.Blocks — every block closes with its named closure, its short form, or `end`,
; which closes the innermost block.
        org 0
macro m1()
endmacro
macro m2()
endm
macro m3()
mend
macro m4()
end
        STRUCT S1
a1      db 0
        endstruct
        STRUCT S2
a1      dw 0
        ends
        STRUCT S3
a1      ds 3
        end
        assert sizeof(S1) + sizeof(S2) + sizeof(S3) == 6
n = 0
        repeat 2
n = n + 1
        endrepeat
        repeat 2
n = n + 1
        rend
        repeat 2
n = n + 1
        end
        while n < 7
n = n + 1
        endwhile
        while n < 8
n = n + 1
        wend
        while n < 9
n = n + 1
        end
        for k = 1 to 1
n = n + 1
        endfor
        for k = 1 to 1
n = n + 1
        end
        if 1
n = n + 1
        endif
        if 1
n = n + 1
        end
        MODULE mo
        ENDMODULE
        MODULE mo
        end
        assert n == 13
