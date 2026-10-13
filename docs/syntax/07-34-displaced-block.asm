; expect-sym: start,label,-,0xA600,0,0x100,
; expect-sym: aligned,label,-,0xA610,0,0x110,
; expect-warning: RUN &A600 falls inside a displaced ORG block
; §7.Displaced blocks — labels take the logical address, bytes go to the storage
; one; `align` aligns the logical address; `run` on it warns.
        org #A600,#100
start:  ld a,1
        nop
        align 16
aligned:
        nop
        run start
        assert start == #A600
        assert aligned == #A610
