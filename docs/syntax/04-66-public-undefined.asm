; expect-error: PUBLIC 'ghost': unknown symbol — nothing in this object defines it
; §4 Expressions, scope between objects — exporting a name nothing defines is refused.
        section code, "ro"
        public ghost
        ret
