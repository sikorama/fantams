; args: --target cpc6128
; expect-sym: data,label,gfx1,0x4010,5,0x4010
; §7.Sections — a section placed by IN is relocatable: an `org` inside it is an
; offset from where the linker puts it.
        section gfx1, "ro" IN w1 OF ext_w1<1>
        org #10
data:   db 1
