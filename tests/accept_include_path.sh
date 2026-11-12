#!/bin/sh
# accept_include_path.sh — le Chemin d'inclusion (CONTEXT.md), vu de la CLI :
# un chemin relatif d'`include` ou d'`incbin` se cherche dans le dossier du fichier qui
# l'ecrit, puis dans chaque `-I` (relatif a l'invocation, ou absolu). Le repertoire
# courant n'a aucun role propre. `fantams` et `ppdump` lisent la meme option.
set -e
cd "$(dirname "$0")/.."
ROOT=$(pwd)
FANTAMS=${FANTAMS:-$ROOT/fantams}
PPDUMP=${PPDUMP:-$ROOT/ppdump}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
fail() { echo "ECHEC : $*"; exit 1; }

mkdir -p "$TMP/proj/sub" "$TMP/lib" "$TMP/elsewhere"
printf '  inc a\n' > "$TMP/proj/sub/leaf.asm"
printf '  INCLUDE "leaf.asm"\n' > "$TMP/proj/sub/mid.asm"
printf '  dec a\n' > "$TMP/lib/util.asm"
printf '  INCLUDE "sub/mid.asm"\n' > "$TMP/proj/nested.asm"
printf '  INCLUDE "sub/mid.asm"\n  INCLUDE "util.asm"\n' > "$TMP/proj/main.asm"
# Un util.asm dans le repertoire d'invocation : il ne doit jamais etre trouve.
printf '  halt\n' > "$TMP/elsewhere/util.asm"

# Lance depuis un AUTRE dossier : les includes se resolvent depuis le fichier, pas d'ici.
cd "$TMP/elsewhere"
"$FANTAMS" ../proj/nested.asm --list-files > "$TMP/out" \
    || fail "un include imbrique ne se resout pas depuis un autre dossier"
for f in ../proj/nested.asm ../proj/sub/mid.asm ../proj/sub/leaf.asm; do
    grep -qxF "$f" "$TMP/out" || { cat "$TMP/out"; fail "$f absent de la Fermeture"; }
done

# Sans -I, util.asm (ni dans proj/ ni a cote de main.asm) est refuse, meme si le
# repertoire d'invocation en contient un ; le message dit ou l'on a cherche.
if "$FANTAMS" ../proj/main.asm --list-files > "$TMP/o2" 2> "$TMP/e2"; then
    fail "le repertoire d'invocation a servi de dossier de recherche"
fi
grep -q 'searched:' "$TMP/e2" || { cat "$TMP/e2"; fail "le refus ne dit pas ou l'on a cherche"; }

# -I relatif a l'invocation, forme separee, puis forme collee ; la Fermeture porte le chemin trouve.
"$FANTAMS" ../proj/main.asm -I ../lib --list-files > "$TMP/o3" || fail "-I dir"
grep -qxF "../lib/util.asm" "$TMP/o3" || { cat "$TMP/o3"; fail "-I relatif : chemin trouve absent"; }
"$FANTAMS" ../proj/main.asm -I../lib --list-files > "$TMP/o4" || fail "-Idir"
cmp -s "$TMP/o3" "$TMP/o4" || fail "-I dir et -Idir different"

# -I absolu, et repetable : le premier dossier qui contient le fichier gagne.
"$FANTAMS" ../proj/main.asm -I "$TMP/nope" -I "$TMP/lib" --list-files > "$TMP/o5" || fail "-I absolu repete"
grep -qxF "$TMP/lib/util.asm" "$TMP/o5" || { cat "$TMP/o5"; fail "-I absolu : chemin trouve absent"; }

# ppdump lit la meme option, et deroule pareil que fantams -E.
"$PPDUMP" ../proj/main.asm -I ../lib > "$TMP/pp" || fail "ppdump -I"
printf 'inc a\ndec a\n' > "$TMP/want"
cmp -s "$TMP/pp" "$TMP/want" || { cat "$TMP/pp"; fail "ppdump -I : source deroulee inattendue"; }
if "$PPDUMP" ../proj/main.asm > /dev/null 2>&1; then fail "ppdump sans -I a accepte un include introuvable"; fi

# incbin suit la meme regle.
printf '\001\002' > "$TMP/lib/blob.bin"
printf '  INCBIN "blob.bin"\n' > "$TMP/proj/bin.asm"
"$PPDUMP" ../proj/bin.asm -I ../lib > "$TMP/bin" || fail "incbin -I"
grep -q '#01,#02' "$TMP/bin" || { cat "$TMP/bin"; fail "incbin : octets inattendus"; }

# -I sans argument : refuse, pas pris pour un fichier source.
if "$FANTAMS" ../proj/main.asm -I > /dev/null 2> "$TMP/e6"; then fail "-I sans argument accepte"; fi

echo "acceptation : le Chemin d'inclusion (dossier du fichier, puis -I) pour fantams et ppdump"
