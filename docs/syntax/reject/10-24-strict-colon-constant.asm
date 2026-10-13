; args: --strict
; expect-error: strict: 'INIT_MSG' is a constant, not a label
; §10.Modules — `--strict` refuses a `:` after a constant: a colon marks a label.
INIT_MSG: EQU 0
