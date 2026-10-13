#!/bin/sh
# §12 Inclusion — include: the included source assembles as if written in place.
set -e
"$FANTAMS" main.asm -o "$TMP/main.bin"
