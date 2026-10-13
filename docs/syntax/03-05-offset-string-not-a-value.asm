; expect-error: a string literal of 5 bytes has no value
; §3 Strings — the offset string is an emission construct, never a value.
        ld a, 'hello'-'a'
