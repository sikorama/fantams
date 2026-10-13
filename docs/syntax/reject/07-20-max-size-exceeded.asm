; expect-error: Section 'audio' exceeds maximum declared size (0x2140 > 0x2000 bytes)
; §7.A declared maximum size — the overflow is reported at assembly time.
        section audio, "ro", 0x2000
        org #4000
        ds #2140
