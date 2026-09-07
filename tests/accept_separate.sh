#!/bin/sh
# accept_separate.sh — le critere de fin de l'etage B.
#
# Deux sources portant une section relocalisable, un PUBLIC / EXTERN, un saut
# relatif inter-sections et un high() : assemblees SEPAREMENT puis linkees,
# elles doivent produire un binaire IDENTIQUE OCTET POUR OCTET a celui de la
# version monolithique equivalente.
#
# Sans ce controle, la compilation separee serait un mecanisme que rien
# n'exerce — l'etat que l'etage A a refuse trois fois.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$FANTAMS" examples/separate_a.asm -o "$TMP/a.fo"    >/dev/null 2>&1
"$FANTAMS" examples/separate_b.asm -o "$TMP/b.fo"    >/dev/null 2>&1
"$FANTAMS" "$TMP/a.fo" "$TMP/b.fo" -o "$TMP/sep.bin" >/dev/null 2>&1
"$FANTAMS" examples/separate_mono.asm -o "$TMP/mono.bin" >/dev/null 2>&1

if cmp -s "$TMP/sep.bin" "$TMP/mono.bin"; then
    echo "acceptation : separe == monolithique, $(wc -c < "$TMP/sep.bin" | tr -d ' ') octets"
else
    echo "ECHEC : le binaire separe differe du monolithique"
    cmp "$TMP/sep.bin" "$TMP/mono.bin" || true
    exit 1
fi

# L'aller-retour du .fo, sur les memes objets.
"$FANTAMS" "$TMP/a.fo" -o "$TMP/a2.fo" >/dev/null 2>&1
if cmp -s "$TMP/a.fo" "$TMP/a2.fo"; then
    echo "acceptation : ecrire, relire, reecrire -> texte identique"
else
    echo "ECHEC : l'aller-retour du .fo n'est pas stable"
    exit 1
fi
