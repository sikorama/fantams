; §3 Strings — 'text' and "text" are one object: 'A' is "A", and each
; delimiter is an ordinary character inside the other.
        org #4000
        assert 'A' == "A"
first:
        db 'a"b', "a'b"
last:
        assert last - first == 6
