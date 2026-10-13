; expect-error: unknown type "code" (the three types are "ro", "rw" and "uninit")
; §7.Sections — a fourth type is refused, naming the three.
        section data, "code"
