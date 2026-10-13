; expect-error: PUBLIC 'other': this name is declared EXTERN
; §4 Expressions, scope between objects — exporting a name declared extern is refused.
        section code, "ro"
        extern other
        public other
