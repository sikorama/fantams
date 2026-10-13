; expect-warning: section 'code' has no 'org' of its own: the linker places it
; §4 Expressions, scope between objects — an org above a section does not place
; it; the linker does, and says so. Write the org inside to place it yourself.
        org  #8000
        section code, "ro"
        nop
