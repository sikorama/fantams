; expect-sym: data,label,-,0x4000,4,0x4000,
; expect-sym: base,label,-,0x8000,2,0x8000,
; expect-warning: a raw binary cannot carry them
; §7.Banks — `b4:` stores in bank 4, the label keeps the logical address;
; without a prefix the bank follows the address. A raw binary cannot carry banks.
        org b4:#4000
data:   db 1
        org b2:#8000
base:   db 2
        assert data == #4000
