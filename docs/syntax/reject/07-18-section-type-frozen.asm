; expect-error: SECTION 'data' was already declared "ro": a section keeps the type of its first declaration
; §7.Sections — a reopening keeps the first type: "ro" then "rw" would disarm
; the write check.
        section data, "ro"
        org #4000
        db 1
        section data, "rw"
