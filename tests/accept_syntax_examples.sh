#!/bin/sh
# accept_syntax_examples.sh — les exemples de docs/syntax/ disent vrai.
#
# Un fichier par point de docs/syntax.md, nommé NN-MM-sujet.asm (NN : chapitre).
# Ceux qui doivent échouer (expect-error) vivent dans docs/syntax/reject/.
# Ses premières lignes de commentaire portent le contrat, que ce script vérifie :
#
#   ; args: <options>            options ajoutées à la ligne de commande
#                                ({dir} : le répertoire de l'exemple)
#   ; expect-error: <texte>      l'assemblage échoue, et la sortie contient <texte>
#   ; expect-warning: <texte>    l'assemblage réussit, et la sortie contient <texte>
#   ; expect-out: <texte>        le fichier -o contient <texte> (avec -E, --beautify…)
#   ; expect-sym: <texte>        la table --sym contient <texte>
#
# Sans expect-error, l'assemblage doit réussir. Les `assert` du source portent le
# reste : un exemple qui affirme une valeur la vérifie lui-même.
#
# Un exemple à plusieurs fichiers est un répertoire NN-MM-sujet/ avec un run.sh,
# lancé avec FANTAMS et TMP dans l'environnement ; il doit sortir en 0.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
case $FANTAMS in /*) ;; *) FANTAMS=$(pwd)/$FANTAMS ;; esac
DIR=docs/syntax
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

failed=0
count=0
fail() { echo "ECHEC $1 : $2" >&2; failed=$((failed + 1)); }

# Les lignes d'en-tête « ; clé: valeur » d'un exemple, pour une clé.
header() { sed -n "s/^; $2: //p" "$1"; }

for src in "$DIR"/*.asm "$DIR"/reject/*.asm; do
    [ -e "$src" ] || continue
    count=$((count + 1))
    name=$(basename "$src" .asm)
    args=$(header "$src" args | sed "s|{dir}|$DIR|g")
    out="$TMP/$name.out"
    log="$TMP/$name.log"
    sym=""
    if [ -n "$(header "$src" expect-sym)" ]; then sym="--sym=$TMP/$name.sym"; fi

    # shellcheck disable=SC2086
    if $FANTAMS "$src" $args $sym -o "$out" >"$log" 2>&1; then rc=0; else rc=$?; fi

    errors=$(header "$src" expect-error)
    if [ -n "$errors" ]; then
        [ $rc != 0 ] || { fail "$name" "devait échouer, a réussi"; continue; }
    else
        [ $rc = 0 ] || { fail "$name" "devait réussir : $(head -3 "$log")"; continue; }
    fi
    printf '%s\n' "$errors" | while IFS= read -r t; do
        [ -z "$t" ] || grep -qF -- "$t" "$log" || echo "message absent : $t"
    done >"$TMP/miss"
    header "$src" expect-warning | while IFS= read -r t; do
        grep -qF -- "$t" "$log" || echo "avertissement absent : $t"
    done >>"$TMP/miss"
    header "$src" expect-out | while IFS= read -r t; do
        grep -qF -- "$t" "$out" 2>/dev/null || echo "sortie sans : $t"
    done >>"$TMP/miss"
    header "$src" expect-sym | while IFS= read -r t; do
        grep -qF -- "$t" "$TMP/$name.sym" 2>/dev/null || echo "--sym sans : $t"
    done >>"$TMP/miss"
    if [ -s "$TMP/miss" ]; then fail "$name" "$(cat "$TMP/miss")"; fi
done

for run in "$DIR"/*/run.sh; do
    [ -e "$run" ] || continue
    count=$((count + 1))
    name=$(basename "$(dirname "$run")")
    mkdir -p "$TMP/$name"
    if ! (cd "$(dirname "$run")" && FANTAMS=$FANTAMS TMP=$TMP/$name sh ./run.sh) \
        >"$TMP/$name.log" 2>&1; then
        fail "$name" "$(tail -3 "$TMP/$name.log")"
    fi
done

if [ $failed != 0 ]; then
    echo "accept_syntax_examples : $failed exemple(s) sur $count en échec" >&2
    exit 1
fi
echo "accept_syntax_examples : $count exemples OK"
