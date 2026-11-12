#!/usr/bin/env bash
# Garde-fou : un parcours de ligne ne décide pas seul si un guillemet ouvre un
# littéral (l'apostrophe de `af'` n'en ouvre pas). Il appelle kw::opensString.
# Refuse le test réécrit à la main sur la variable de boucle `c` : guillemet
# double, puis apostrophe, sur la même ligne. keywords.cpp porte le prédicat.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
bad=$(grep -nP '(^|[^.A-Za-z_])c == \x27"\x27 \|\|.*\x27\\\x27\x27' -- *.cpp *.h |
    grep -v '^keywords\.cpp:' || true)
if [ -n "$bad" ]; then
    echo "check-quotes : test de guillemet réécrit à la main, utiliser kw::opensString :" >&2
    echo "$bad" >&2
    exit 1
fi
