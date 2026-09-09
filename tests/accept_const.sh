#!/bin/sh
# accept_const.sh — l'etage E, etape E6.
#
# Deux affirmations de l'ADR 0032 ne se verifient qu'ICI, en sortant du
# processus reel, parce qu'elles portent sur ce que la ligne de commande
# assemble et lie — pas sur ce qu'un profil fabrique a la main dans un test
# unitaire rend :
#
#   1. un CONST declare dans un profil est visible du SOURCE, directement,
#      sans EXTERN — le meme chemin que le triplet __port_/__val_/__mask_ ;
#   2. un CONST est aussi lisible DANS le SELECT qui l'utilise, et c'est ce
#      qui retire le port en dur des ecritures de commutation.
#
# Et une troisieme, l'autre moitie de la decision E2 : un profil qui ne
# declare AUCUN CONST continue d'emettre __port_ comme avant cet etage.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/const.prof" <<'EOF'
TARGET test_const
WINDOW w0 [0x0000..0x3FFF]
BANK b0 SIZE 0x4000 rw STORE 0
CONST GA_PORT = 0x7F00
CONFIG SET ram { s [CODE 0] { w0 b0 } }
SELECT ram = OUT GA_PORT, CODE
EOF

cat > "$TMP/const.ld" <<'EOF'
MEMORY_MAP {
    CONFIG s { w0 { SECTION boot } }
}
EOF

# 1. GA_PORT est un CONST : visible DU SOURCE, sans EXTERN.
cat > "$TMP/a.asm" <<'EOF'
section boot, "ro"
        ld   bc, GA_PORT
EOF
"$FANTAMS" "$TMP/a.asm" -P "$TMP/const.prof" -T "$TMP/const.ld" -o "$TMP/a.bin" >/dev/null 2>&1

head=$(od -An -tx1 -N 3 "$TMP/a.bin" | tr -d ' \n')
case "$head" in
    01007f) ;;
    *) echo "ECHEC : GA_PORT au source rend $head, attendu 01007f (ld bc,&7F00)"
       exit 1 ;;
esac
echo "acceptation : un CONST du profil est visible du source, sans EXTERN"

# 2. GA_PORT est aussi lu PAR le SELECT qui l'ecrit : __port_ram_boot vaut le
#    meme nombre, sans qu'aucune ligne du profil n'ecrive 0x7F00 deux fois.
cat > "$TMP/b.asm" <<'EOF'
section boot, "ro"
        ld   bc, __port_ram_boot
EOF
"$FANTAMS" "$TMP/b.asm" -P "$TMP/const.prof" -T "$TMP/const.ld" -o "$TMP/b.bin" >/dev/null 2>&1

head=$(od -An -tx1 -N 3 "$TMP/b.bin" | tr -d ' \n')
case "$head" in
    01007f) ;;
    *) echo "ECHEC : __port_ram_boot rend $head, attendu 01007f — le SELECT n'a pas lu GA_PORT"
       exit 1 ;;
esac
echo "acceptation : un CONST se lit aussi DANS le SELECT qui l'ecrit"

# 3. E2 : un profil SANS CONST continue d'emettre __port_ comme avant.
sed 's/^CONST GA_PORT = 0x7F00$/SELECT ram = OUT 0x7F00, CODE/; s/^SELECT ram = OUT GA_PORT, CODE$//' \
    "$TMP/const.prof" > "$TMP/no_const.prof"
"$FANTAMS" "$TMP/b.asm" -P "$TMP/no_const.prof" -T "$TMP/const.ld" -o "$TMP/c.bin" >/dev/null 2>&1
if ! cmp -s "$TMP/b.bin" "$TMP/c.bin"; then
    echo "ECHEC : un profil sans CONST ne rend plus les memes octets pour __port_"
    exit 1
fi
echo "acceptation : un profil sans CONST continue d'emettre __port_ a l'identique"
