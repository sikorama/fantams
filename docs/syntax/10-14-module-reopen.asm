; §10.Modules — reopening a module sees the constants and labels of its other blocks.
        org 0
        MODULE a
K       EQU 123
        jp fwd                  ; a label of a later block of the same module
        ENDMODULE
        MODULE a
        assert K == 123
fwd:    nop
        ENDMODULE
        assert a.fwd == 3
