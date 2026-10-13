; §3 Strings — a ';' inside a string does not open a comment.
        org #4000
first:
        db 'a;b'        ; three bytes
last:
        assert last - first == 3
        assert ';' == #3B
