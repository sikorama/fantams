; args: --strict
; expect-error: strict: 'DEFB' is a non-canonical spelling — write 'DB'
; §10.Modules — `--strict` refuses `DEFB`… as a struct field's directive, as in code.
        STRUCT S
x1      defb 0
        ENDSTRUCT
