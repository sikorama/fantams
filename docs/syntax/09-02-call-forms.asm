; expect-warning: 'pair' is called without parentheses; write 'pair(...)'
; §9.Call — parenthesized, bare (inherited, warns once per macro), and empty calls.
; A first argument in parentheses doubles them: `pair((4),12)`.
        org 0
macro pair(x1,y1)
sum = sum + x1 + y1
endmacro
macro none()
sum = sum + 100
endmacro
sum = 0
        pair(1,2)
        pair 3,4
        pair 3,4
        none()
        pair((4),12)
        assert sum == 1+2 + 3+4 + 3+4 + 100 + 4+12
