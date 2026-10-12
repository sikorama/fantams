#!/usr/bin/env bash
# Vérifie (ou applique) le format clang-format sur les LIGNES modifiées seulement.
# Le code en place n'est pas formaté de bout en bout (calé à la main, ~4800 écarts) :
# un contrôle sur tout l'arbre noierait le diff. Les lignes que l'on touche, elles,
# doivent être conformes ; un fichier non suivi est contrôlé en entier.
#
# Usage :
#   scripts/format.sh [--check] [BASE]   # défaut BASE=HEAD ; sort != 0 si non conforme
#   scripts/format.sh --fix [BASE]       # réécrit les lignes modifiées
#   scripts/format.sh --staged           # l'index seul (hook pre-commit, make hooks)
#
# Outil : version ÉPINGLÉE (CF_VERSION). Deux versions majeures de clang-format ne
# formatent pas à l'identique ; on n'utilise donc que la version épinglée —
# $CLANG_FORMAT, puis PATH si elle correspond exactement, sinon une installation
# unique depuis PyPI dans ~/.cache (marche aussi dans une distrobox).
set -euo pipefail

cd "$(dirname "$0")/.."

CF_VERSION="22.1.8"
CF_CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/amspirit-lite/clang-format-$CF_VERSION"
CF_PKG="$CF_CACHE/clang_format"

cf_version_of() { "$1" --version 2>/dev/null | sed -n 's/.*clang-format version \([0-9][0-9.]*\).*/\1/p'; }

find_tool() {
    local c
    for c in "${CLANG_FORMAT:-}" "$(command -v clang-format 2>/dev/null || true)" "$CF_PKG/data/bin/clang-format"; do
        [ -n "$c" ] && [ -x "$c" ] && [ "$(cf_version_of "$c")" = "$CF_VERSION" ] && { echo "$c"; return; }
    done
    echo "clang-format $CF_VERSION absent : installation dans $CF_CACHE ..." >&2
    python3 -m pip install --quiet --disable-pip-version-check --no-deps \
        --target "$CF_CACHE" "clang-format==$CF_VERSION" >&2 || return 1
    c="$CF_PKG/data/bin/clang-format"
    [ -x "$c" ] && [ "$(cf_version_of "$c")" = "$CF_VERSION" ] && { echo "$c"; return; }
    return 1
}
CF="$(find_tool)" || {
    echo "error: clang-format $CF_VERSION introuvable et installation PyPI impossible." >&2
    echo "  pip install --target $CF_CACHE clang-format==$CF_VERSION" >&2
    echo "  ou exporter CLANG_FORMAT=/chemin/vers/clang-format ($CF_VERSION)" >&2
    exit 127
}
DIFF_PY="$CF_PKG/clang_format_diff.py"
[ -f "$DIFF_PY" ] || { echo "error: $DIFF_PY introuvable (wheel clang-format $CF_VERSION)." >&2; exit 127; }

MODE=check; STAGED=0; BASE=HEAD
for a in "$@"; do
    case "$a" in
        --check) MODE=check ;;
        --fix|-i) MODE=fix ;;
        --staged) STAGED=1 ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        -*) echo "error: argument inconnu '$a'" >&2; exit 2 ;;
        *) BASE="$a" ;;
    esac
done

SRC='\.(cpp|h)$'
if [ "$STAGED" = 1 ]; then DIFF_ARGS=(--cached "$BASE"); else DIFF_ARGS=("$BASE"); fi
# clang_format_diff.py sort != 0 quand il y a un écart : le patch seul fait foi.
PATCH="$(git diff -U0 "${DIFF_ARGS[@]}" -- '*.cpp' '*.h' | python3 "$DIFF_PY" -p1 -binary "$CF" || true)"

# Fichiers non suivis (hors mode --staged, où l'index fait foi) : contrôlés en entier.
NEW=()
if [ "$STAGED" = 0 ]; then
    while IFS= read -r f; do NEW+=("$f"); done < <(git ls-files --others --exclude-standard | grep -E "$SRC" || true)
fi

if [ "$MODE" = fix ]; then
    git diff -U0 "${DIFF_ARGS[@]}" -- '*.cpp' '*.h' | python3 "$DIFF_PY" -p1 -i -binary "$CF" || true
    if [ "${#NEW[@]}" -gt 0 ]; then "$CF" -i "${NEW[@]}"; fi
    echo "Formatage appliqué ($("$CF" --version))."
    exit 0
fi

BAD=0
if [ -n "$PATCH" ]; then
    echo "Lignes modifiées non conformes ($("$CF" --version)) :" >&2
    printf '%s\n' "$PATCH" >&2
    BAD=1
fi
if [ "${#NEW[@]}" -gt 0 ] && ! "$CF" --dry-run --Werror "${NEW[@]}" 2>/dev/null; then
    echo "Fichiers non suivis non conformes : ${NEW[*]}  (scripts/format.sh --fix)" >&2
    BAD=1
fi
if [ "$BAD" = 0 ]; then echo "Lignes modifiées conformes."; fi
exit "$BAD"
