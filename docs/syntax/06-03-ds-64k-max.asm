; expect-error: DS: 65537 bytes do not fit the 64K address space
; §6 Emission directives — one `ds` is at most #10000 bytes, the whole address
; space; past that it is refused rather than wrapped.
        org 0
        ds #10001
