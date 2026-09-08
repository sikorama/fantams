#!/bin/sh
# accept_wasm_equiv.sh — le verrou : natif ≡ WASM.
#
# Le meme argv et les memes fichiers, a travers l'adaptateur NATIF et a travers
# l'adaptateur WASM, doivent rendre les MEMES OCTETS. C'est la propriete que la
# couture rend disponible gratuitement — l'adaptateur WASM n'exporte aucune
# fonction du coeur, son contrat EST le contrat CLI — et c'est celle qu'un
# artefact perime viole.
#
# Elle remplace le critere qui etait en place, lequel se contentait de verifier
# la signature du snapshot et un drapeau de succes : un artefact vieux de trois
# etages le satisfait sans difficulte.
#
# DISCIPLINE DE SAUT. Deux dependances vivent hors de l'outillage de
# construction de fantams : l'artefact WASM (produit par un conteneur emsdk) et
# l'executable node. Absentes, ce test se SAUTE — code 77, que ctest rapporte
# comme « Skipped » — au lieu d'echouer. Un saut n'est pas un succes : il est
# bruyant dans le rapport, et tout le verrou repose sur cette distinction.
set -e
cd "$(dirname "$0")/.."
ROOT=$(pwd)
FANTAMS=${FANTAMS:-./fantams}

skip() { echo "SAUTE : $1 — natif ≡ WASM non verifie"; exit 77; }

# L'artefact WASM : la destination par defaut de build-wasm.sh, ou celle que
# l'appelant nomme. fantams ne versionne pas ses artefacts de construction :
# ce sont des produits, et l'absence est le cas ordinaire.
WASM_MJS=${FANTAMS_WASM:-$ROOT/dist/fantams.mjs}
[ -f "$WASM_MJS" ] || skip "artefact WASM absent ($WASM_MJS)"
[ -f "${WASM_MJS%.mjs}.wasm" ] || skip "fantams.wasm absent a cote de $WASM_MJS"
command -v node >/dev/null 2>&1 || skip "node absent"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

wasm() { node "$ROOT/tests/wasm_cli.mjs" "$WASM_MJS" "$@"; }

fail=0
# Un cas : le meme argv des deux cotes, et cmp sur l'artefact produit.
# L'EXTENSION de la sortie compte — c'est elle qui aiguille le format (ADR 0007)
# — donc elle est la meme des deux cotes.
#   $1 etiquette  $2 nom lisible  $3 source  $4 extension  $5... le reste de l'argv
case_bytes() {
    tag=$1; name=$2; src=$3; ext=$4; shift 4
    "$FANTAMS" "$src" -o "$TMP/$tag-nat$ext" "$@" >/dev/null 2>&1 || {
        echo "ECHEC : l'adaptateur natif refuse le cas « $name »"; fail=1; return; }
    wasm --in "$ROOT/$src:/in.asm" --out "/out$ext:$TMP/$tag-wasm$ext" \
         -- /in.asm -o "/out$ext" "$@" >/dev/null 2>&1 || {
        echo "ECHEC : l'adaptateur WASM refuse le cas « $name »"; fail=1; return; }
    if cmp -s "$TMP/$tag-nat$ext" "$TMP/$tag-wasm$ext"; then
        echo "acceptation : natif ≡ WASM sur « $name », $(wc -c < "$TMP/$tag-nat$ext" | tr -d ' ') octets"
    else
        echo "ECHEC : natif et WASM divergent sur « $name »"
        cmp "$TMP/$tag-nat$ext" "$TMP/$tag-wasm$ext" || true
        fail=1
    fi
}

# Le binaire brut, et le snapshot : deux sorties, deux chemins de code.
case_bytes demo "demo -> .bin" examples/demo.asm .bin
case_bytes demo "demo -> .sna" examples/demo.asm .sna

# La sortie deroulee (-E) : du TEXTE, ou une divergence de preprocesseur se voit.
# Gardees comme tous les autres cas : sous « set -e », un echec non garde
# sortirait du script ici meme, sans diagnostic et sans jouer les deux cas
# suivants — le cas banque et --version, c'est-a-dire ceux pour lesquels ce
# fichier a ete ecrit.
"$FANTAMS" examples/demo.asm -E -o "$TMP/pp.nat" >/dev/null 2>&1 || {
    echo "ECHEC : l'adaptateur natif refuse -E"; fail=1; }
wasm --in "$ROOT/examples/demo.asm:/in.asm" --out "/out.asm:$TMP/pp.wasm" -- /in.asm -E -o /out.asm >/dev/null 2>&1 || {
    echo "ECHEC : l'adaptateur WASM refuse -E"; fail=1; }
if cmp -s "$TMP/pp.nat" "$TMP/pp.wasm" 2>/dev/null; then
    echo "acceptation : natif ≡ WASM sur la source deroulee, $(wc -l < "$TMP/pp.nat" | tr -d ' ') lignes"
else
    echo "ECHEC : natif et WASM divergent sur la source deroulee"; fail=1
fi

# Un profil de cible et un script de lien : les capacites de l'etage C1. C'est
# le cas qui distingue un artefact a jour d'un artefact d'avant l'etage — les
# quatre modules qui manquaient a la liste de sources sont ceux-la.
"$FANTAMS" examples/banked.asm -T examples/banked.ld --target cpc6128 -o "$TMP/bank.nat" >/dev/null 2>&1 || {
    echo "ECHEC : l'adaptateur natif refuse le cas banque"; fail=1; }
wasm --in "$ROOT/examples/banked.asm:/in.asm" --in "$ROOT/examples/banked.ld:/in.ld" \
     --out "/out.bin:$TMP/bank.wasm" -- /in.asm -T /in.ld --target cpc6128 -o /out.bin >/dev/null 2>&1 || {
    echo "ECHEC : l'adaptateur WASM refuse le cas banque (etage C1 absent de l'artefact ?)"; fail=1; }
if [ -f "$TMP/bank.wasm" ] && cmp -s "$TMP/bank.nat" "$TMP/bank.wasm"; then
    echo "acceptation : natif ≡ WASM sur le cas banque (profil + script de lien)"
else
    echo "ECHEC : natif et WASM divergent sur le cas banque"; fail=1
fi

# La VERSION, a travers les deux adaptateurs. On compare la FORME et la date de
# version — pas la date de compilation : les deux artefacts ne sont pas compiles
# au meme moment, et c'est precisement ce que l'ecart est fait pour dire.
# « || true » : un artefact qui ne connait pas --version doit etre RAPPORTE,
# pas faire sortir le script avant ses diagnostics.
vnat=$("$FANTAMS" --version 2>&1 || true)
vwasm=$(wasm -- --version 2>&1 || true)
form='^fantams [0-9]{4}-[0-9]{2}-[0-9]{2} [(]compile [0-9]{4}-[0-9]{2}-[0-9]{2}[)]$'
# Un artefact qui ne connait pas l'option repond n'importe quoi, et un module
# qui ne se charge pas repond une trace de plusieurs milliers de lignes. On
# rapporte la PREMIERE ligne : elle suffit a nommer la panne, et le rapport de
# la suite reste lisible.
first() { echo "$1" | head -1; }
if ! echo "$vnat" | grep -Eq "$form"; then echo "ECHEC : forme de --version natif : $(first "$vnat")"; fail=1; fi
if ! echo "$vwasm" | grep -Eq "$form"; then echo "ECHEC : forme de --version WASM : $(first "$vwasm")"; fail=1; fi
# On extrait D'ABORD la ligne de version, ensuite seulement on en prend le
# second champ. Sans cela, la moindre ligne parasite sur la sortie d'erreur —
# un avertissement experimental de node, une note du moteur WASM — rend « cut »
# multiligne et fait accuser l'artefact d'etre perime alors que rien ne l'est.
release_of() { echo "$1" | grep -E "$form" | head -1 | cut -d' ' -f2; }
rnat=$(release_of "$vnat")
rwasm=$(release_of "$vwasm")
if [ "$rnat" = "$rwasm" ]; then
    echo "acceptation : --version, meme forme et meme date de version des deux cotes ($rnat)"
else
    echo "ECHEC : date de version natif=${rnat:-<aucune>} wasm=${rwasm:-<aucune>} — l'artefact WASM est perime"
    echo "        natif : $(first "$vnat")"
    echo "        wasm  : $(first "$vwasm")"
    fail=1
fi

exit $fail
