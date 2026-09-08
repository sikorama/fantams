#!/bin/sh
# accept_aliased.sh — quatre banques a la MEME adresse logique, et les deux
# facons de les placer.
#
# `accept_banked.sh` exerce UNE banque etendue. Des qu'un programme depasse
# 64 K, il en emploie plusieurs, toutes vues par la meme fenetre : c'est la
# forme reelle, et c'est celle-ci.
#
# Le controle en titre est le n°2. Une source qui place ses banques A LA MAIN
# (`org bN:`, valeurs de commutation en `equ`) et la MEME source qui laisse le
# linker placer (`SECTION` + script) doivent rendre les MEMES OCTETS. Aucun
# oracle exterieur n'est convoque : ce sont deux chemins du meme outil, et
# c'est leur accord qui fait la preuve.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

A=examples/aliased.asm
L=examples/aliased.ld
O=examples/aliased_org.asm

# 1. La source liee ne nomme AUCUN emplacement, et sa jumelle les nomme TOUS.
#    Les deux moities de l'enonce, verifiees plutot qu'affirmees.
if grep -nE '^[^;]*\b(org|BANK)\b' "$A" >/dev/null; then
    echo "ECHEC : $A nomme un emplacement"; exit 1
fi
if ! grep -qE '^[^;]*\borg\s+b[4-7]:' "$O"; then
    echo "ECHEC : $O ne place plus ses banques a la main"; exit 1
fi

"$FANTAMS" "$A" -T "$L" -o "$TMP/ld.sna"  --sym="$TMP/ld.sym"  >/dev/null 2>&1
"$FANTAMS" "$O"           -o "$TMP/org.sna"                    >/dev/null 2>&1

# 2. LE CONTROLE EN TITRE.
if cmp -s "$TMP/ld.sna" "$TMP/org.sna"; then
    echo "acceptation : place par le linker == place a la main, $(wc -c < "$TMP/ld.sna" | tr -d ' ') octets"
else
    echo "ECHEC : les deux placements divergent"
    cmp -l "$TMP/ld.sna" "$TMP/org.sna" | head -5 || true
    exit 1
fi

sym()  { grep "^$1," "$TMP/ld.sym" | cut -d, -f4; }
bank() { grep "^$1," "$TMP/ld.sym" | cut -d, -f5; }

# 3. L'ALIASING : quatre banques distinctes, une seule adresse logique.
#    C'est ce qu'aucune derivation par l'adresse ne saurait produire.
for pair in gfx0_data:4 gfx1_data:5 gfx2_data:6 gfx3_data:7; do
    n=${pair%:*}; want=${pair#*:}
    [ "$(bank "$n")" = "$want" ] || { echo "ECHEC : $n en banque $(bank "$n"), attendu $want"; exit 1; }
    [ "$(sym  "$n")" = "0x4000" ] || { echo "ECHEC : $n vaut $(sym "$n"), attendu 0x4000"; exit 1; }
done
echo "acceptation : gfx0..gfx3 en banques 4,5,6,7 et TOUTES a 0x4000"

# 4. Les valeurs de commutation sont dans les octets emis, et ce sont celles
#    que le profil dicte : OUT 0x7F00, %11000000 | CODE, CODE = %100|b.
#    `ld bc, __port_ram_gfx0 + __val_ram_gfx0` -> 01 C4 7F, puis 0xC5, 0xC6, 0xC7.
#    `resident` est en banque 2 : offset 256 + 2*0x4000.
head=$(od -An -tx1 -j $((256 + 2 * 16384)) -N 6 "$TMP/ld.sna" | tr -d ' \n')
case "$head" in
    1100c001c47f) ;;
    *) echo "ECHEC : resident commence par $head"
       echo "        attendu 1100c001c47f — ld de,&C000 / ld bc,&7FC4"
       exit 1 ;;
esac
echo "acceptation : la valeur de commutation emise est celle du profil (&7FC4)"

# 5. LE CONTROLE QUI COMPTE. Permuter deux sections DANS LE SCRIPT SEUL doit
#    echanger leurs banques et leurs valeurs de commutation, sans toucher UNE
#    adresse logique — et les octets doivent changer, faute de quoi le script
#    ne pilotait rien.
sed -e 's/SECTION gfx0/SECTION gfxA/' -e 's/SECTION gfx3/SECTION gfx0/' \
    -e 's/SECTION gfxA/SECTION gfx3/' "$L" > "$TMP/swapped.ld"
"$FANTAMS" "$A" -T "$TMP/swapped.ld" -o "$TMP/sw.sna" --sym="$TMP/sw.sym" >/dev/null 2>&1

swbank() { grep "^$1," "$TMP/sw.sym" | cut -d, -f5; }
[ "$(swbank gfx0_data)" = "7" ] || { echo "ECHEC : gfx0 n'a pas suivi le script (banque $(swbank gfx0_data))"; exit 1; }
[ "$(swbank gfx3_data)" = "4" ] || { echo "ECHEC : gfx3 n'a pas suivi le script (banque $(swbank gfx3_data))"; exit 1; }

# Les adresses logiques : identiques. On compare des ENSEMBLES, d'ou le `sort` —
# permuter deux sections reordonne les lignes de la table, qui suit le placement.
# C'est un fait sur l'ordre du fichier, pas sur les adresses, et confondre les
# deux ferait echouer le test sur la propriete qu'il est cense defendre.
#
# Fichiers temporaires et non substitution de processus : ce script tourne aussi
# sous un /bin/sh qui n'est pas bash (meme raison que dans accept_banked.sh).
cut -d, -f1,4 "$TMP/ld.sym" | sort > "$TMP/a"
cut -d, -f1,4 "$TMP/sw.sym" | sort > "$TMP/b"
if ! cmp -s "$TMP/a" "$TMP/b"; then
    echo "ECHEC : permuter deux sections a change une adresse logique"
    diff "$TMP/a" "$TMP/b" || true
    exit 1
fi
if cmp -s "$TMP/ld.sna" "$TMP/sw.sna"; then
    echo "ECHEC : permuter deux sections n'a rien change aux octets"
    exit 1
fi
echo "acceptation : permuter gfx0 et gfx3 echange leurs banques et leurs valeurs"
echo "              de commutation, et pas une adresse logique"
