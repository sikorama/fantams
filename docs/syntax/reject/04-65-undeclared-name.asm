; expect-error: unknown symbol 'helpr'
; §4 Expressions, scope between objects — a name neither defined nor declared
; extern stays an assembly error, at its own line: no implicit extern.
        section code, "ro"
        call helpr
