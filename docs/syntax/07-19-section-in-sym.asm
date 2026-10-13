; expect-sym: lab,label,code,0x100,
; expect-sym: K,const,-,
; §7.Sections — `--sym` names the section that owns each label; a constant lives
; nowhere and carries none.
        section code, "ro"
        org #100
K       equ 3
lab:    nop
