#!/bin/sh
# accept_list_files.sh — `--list-files` : la Fermeture d'un point d'entree
# (z80live CONTEXT.md, ADR 0002), rendue par la CLI. Meme esprit que
# --dump-profile : imprime une donnee que le preprocesseur connait deja
# (pp::Result::files()), sans fabriquer de nouveau moteur.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

printf '  inc a\n' > "$TMP/b.asm"
printf '  nop\n  INCLUDE "%s/b.asm"\n' "$TMP" > "$TMP/a.asm"
printf '  ld a,1\n  INCLUDE "%s/a.asm"\n' "$TMP" > "$TMP/main.asm"

"$FANTAMS" "$TMP/main.asm" --list-files > "$TMP/out"

# Le fichier principal, plus chaque INCLUDE touche RECURSIVEMENT, un par ligne.
n=$(wc -l < "$TMP/out" | tr -d ' ')
if [ "$n" != 3 ]; then
    echo "ECHEC : attendu 3 fichiers, obtenu $n"
    cat "$TMP/out"
    exit 1
fi
for f in "$TMP/main.asm" "$TMP/a.asm" "$TMP/b.asm"; do
    if ! grep -qxF "$f" "$TMP/out"; then
        echo "ECHEC : $f absent de la Fermeture"
        cat "$TMP/out"
        exit 1
    fi
done

# Aucun octet assemble ni lie : --list-files s'arrete a la Fermeture.
if [ -e "$TMP/main.bin" ]; then
    echo "ECHEC : --list-files a quand meme ecrit un binaire"
    exit 1
fi

# Un INCLUDE manquant est refuse (comme n'importe quel preprocessing), rien
# sur la sortie standard : une Fermeture partielle ne doit pas ressembler a
# une Fermeture complete.
printf '  INCLUDE "absent.asm"\n' > "$TMP/bad.asm"
if "$FANTAMS" "$TMP/bad.asm" --list-files > "$TMP/badout" 2>"$TMP/err"; then
    echo "ECHEC : un include manquant a ete accepte"
    exit 1
fi
if [ -s "$TMP/badout" ]; then
    echo "ECHEC : une Fermeture partielle est sortie malgre l'echec"
    exit 1
fi
if ! grep -q 'include not found' "$TMP/err"; then
    echo "ECHEC : le refus ne nomme pas l'include manquant"
    cat "$TMP/err"
    exit 1
fi

# Un .fo (deja assemble) n'a plus l'information de provenance : refuse plutot
# que de rendre une Fermeture inventee ou silencieusement vide.
"$FANTAMS" "$TMP/b.asm" -o "$TMP/b.fo" >/dev/null 2>&1
if "$FANTAMS" "$TMP/b.fo" --list-files > "$TMP/foout" 2>"$TMP/foerr"; then
    echo "ECHEC : --list-files sur un .fo a ete accepte"
    exit 1
fi

echo "acceptation : --list-files rend la Fermeture (principal + includes recursifs, dedupliques)"
