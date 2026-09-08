#!/bin/sh
# accept_profile.sh — le critere de l'etape C1.3.
#
# Trois affirmations de la decision D1 ne se verifient qu'ICI, en sortant du
# processus, parce qu'elles portent sur ce que la ligne de commande rend :
#
#   1. `--dump-profile` est une COPIE, pas une serialisation — il n'existe aucun
#      ecrivain qui pourrait diverger de l'analyseur ;
#   2. le texte exporte se RELIT, ce qui est la seule preuve que la copie est
#      utilisable et pas seulement identique ;
#   3. le profil embarque n'a AUCUN PRIVILEGE : `--target N` et `-P <copie de N>`
#      passent par le meme chemin de code, et rendent le meme octet.
#
# Et une quatrieme, qui est l'engagement du §12.1 : sans profil du tout, les
# octets sont ceux d'avant l'etage.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 1. L'export est deterministe : deux appels, le meme texte.
"$FANTAMS" --dump-profile cpc6128 > "$TMP/a.prof"
"$FANTAMS" --dump-profile cpc6128 > "$TMP/b.prof"
if ! cmp -s "$TMP/a.prof" "$TMP/b.prof"; then
    echo "ECHEC : --dump-profile n'est pas deterministe"
    exit 1
fi
if [ ! -s "$TMP/a.prof" ]; then
    echo "ECHEC : --dump-profile n'a rien rendu"
    exit 1
fi

# Les citations FONT PARTIE du profil : un export qui les perdrait rendrait un
# profil que personne ne peut auditer (§12.3). C'est ce qu'un serialiseur aurait
# fait, et la raison de n'en avoir aucun.
if ! grep -q 'NON TRANCHE' "$TMP/a.prof"; then
    echo "ECHEC : le profil exporte a perdu ses citations"
    exit 1
fi

# 2. et 3. Le texte exporte se relit, et il ne se distingue pas du texte
# embarque : trois invocations, un seul binaire attendu.
"$FANTAMS" examples/demo.asm --target cpc6128 -o "$TMP/builtin.bin" >/dev/null 2>&1
"$FANTAMS" examples/demo.asm -P "$TMP/a.prof" -o "$TMP/copy.bin"   >/dev/null 2>&1
"$FANTAMS" examples/demo.asm -o "$TMP/none.bin"                    >/dev/null 2>&1

if ! cmp -s "$TMP/builtin.bin" "$TMP/copy.bin"; then
    echo "ECHEC : le profil embarque et sa copie ne rendent pas le meme binaire"
    exit 1
fi
if ! cmp -s "$TMP/builtin.bin" "$TMP/none.bin"; then
    echo "ECHEC : un profil seul a change les octets ; a cet etage il ne doit rien placer"
    exit 1
fi

# Un nom inconnu est refuse, et le refus LISTE ce qui existe.
if "$FANTAMS" examples/demo.asm --target pas_une_machine -o "$TMP/x.bin" 2>"$TMP/err"; then
    echo "ECHEC : un --target inconnu a ete accepte"
    exit 1
fi
if ! grep -q 'cpc6128' "$TMP/err"; then
    echo "ECHEC : le refus d'un --target inconnu ne liste pas les profils livres"
    exit 1
fi

# Un profil fautif est refuse, et le diagnostic nomme SA ligne — un profil est
# ecrit par une personne.
printf 'WINDOW w0 [0x0000..0x3FFF]\nBANK b0 rw\n' > "$TMP/bad.prof"
if "$FANTAMS" examples/demo.asm -P "$TMP/bad.prof" -o "$TMP/x.bin" 2>"$TMP/err2"; then
    echo "ECHEC : un profil sans SIZE a ete accepte"
    exit 1
fi
if ! grep -q ':2: error' "$TMP/err2"; then
    echo "ECHEC : le refus d'un profil fautif ne nomme pas sa ligne"
    cat "$TMP/err2"
    exit 1
fi

echo "acceptation : profil embarque == sa copie == aucun profil, $(wc -c < "$TMP/a.prof" | tr -d ' ') octets de texte"
