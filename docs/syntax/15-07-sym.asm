; expect-sym: name,type,section,value,bank,store,file,line
; expect-sym: start,label,-,0x4000,1,0x4000,
; expect-sym: start.loop,label,-,0x4002,1,0x4002,
; expect-sym: K,const,-,0x5,-,-,
; §15 Modes — `--sym` writes one CSV line per label and constant: name, type,
; section, logical value, storage bank and address, origin file and line.
    org #4000
K equ 5
v = 3
start:
    ld b,K
.loop:
    djnz .loop
