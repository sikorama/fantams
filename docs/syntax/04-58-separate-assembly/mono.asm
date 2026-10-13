        section code, "ro"
start:
        call helper
        ret
        section lib, "ro"
helper:
        ld   a, 1
        ret
