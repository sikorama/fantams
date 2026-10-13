; §3 Strings — offset string: 'db' applies the arithmetic tail to each byte,
; so 'hello'-'a' emits five bytes, the first one 'h'-'a' = 7.
        org #4000
first:
        db 'hello'-'a'
last:
        assert last - first == 5
        assert 'h' - 'a' == 7
