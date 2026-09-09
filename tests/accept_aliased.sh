#!/bin/sh
# accept_aliased.sh — quatre banques a la MEME adresse logique, et les deux
# facons de les placer.
#
# `accept_banked.sh` exerce UNE banque etendue. Des qu'un programme depasse
# 64 K, il en emploie plusieurs, toutes vues par la meme fenetre : c'est la
# forme reelle, et c'est celle-ci.
#
# Le controle en titre est le n°2, et il porte sur TROIS chemins du meme outil :
#
#   aliased.asm     + aliased.ld : le source ne place rien, le script place ;
#   aliased_org.asm             : le source place a la main, `org bN:` et `equ` ;
#   aliased_sym.asm             : le source place par ses SECTIONS, et le linker
#                                 lui rend les valeurs de commutation.
#
# Les trois doivent rendre les MEMES OCTETS. Aucun oracle exterieur n'est
# convoque : c'est leur accord qui fait la preuve.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

A=examples/aliased.asm
L=examples/aliased.ld
O=examples/aliased_org.asm
S=examples/aliased_sym.asm

# 1. Chaque source dit ce qu'elle est, et on le VERIFIE plutot que de
#    l'affirmer : la premiere ne nomme aucun emplacement, la deuxieme les nomme
#    tous a la main, la troisieme place ses sections SANS ecrire un seul nombre.
if grep -nE '^[^;]*\b(org|BANK)\b' "$A" >/dev/null; then
    echo "ECHEC : $A nomme un emplacement"; exit 1
fi
if ! grep -qE '^[^;]*\borg\s+b[4-7]:' "$O"; then
    echo "ECHEC : $O ne place plus ses banques a la main"; exit 1
fi
if ! grep -qE '^[^;]*\bSECTION\b.*\bIN\b' "$S"; then
    echo "ECHEC : $S ne place plus ses sections"; exit 1
fi
if grep -nE '^[^;]*\b(org|equ)\b' "$S" >/dev/null; then
    echo "ECHEC : $S ecrit un org ou un equ — les valeurs doivent venir du linker"
    grep -nE '^[^;]*\b(org|equ)\b' "$S" | head -3
    exit 1
fi

"$FANTAMS" "$A" -T "$L" -o "$TMP/ld.sna"  --sym="$TMP/ld.sym"  >/dev/null 2>&1
"$FANTAMS" "$O"           -o "$TMP/org.sna"                    >/dev/null 2>&1
"$FANTAMS" "$S" --target cpc6128 -o "$TMP/sym.sna" --sym="$TMP/sym.sym" >/dev/null 2>&1

# 2. LE CONTROLE EN TITRE : les trois chemins, octet pour octet.
if ! cmp -s "$TMP/ld.sna" "$TMP/org.sna"; then
    echo "ECHEC : place par le linker et place a la main divergent"
    cmp -l "$TMP/ld.sna" "$TMP/org.sna" | head -5 || true
    exit 1
fi
if ! cmp -s "$TMP/ld.sna" "$TMP/sym.sna"; then
    echo "ECHEC : place par le linker et place par les sections divergent"
    cmp -l "$TMP/ld.sna" "$TMP/sym.sna" | head -5 || true
    exit 1
fi
echo "acceptation : script == a la main == sections, $(wc -c < "$TMP/ld.sna" | tr -d ' ') octets"

# 2 bis. Et les valeurs de commutation du troisieme viennent bien du PROFIL :
#        les memes octets qu'ailleurs, alors qu'aucun `equ` ne les ecrit. Le
#        controle n°4 les lit ; ici on verifie que la table des symboles place
#        les quatre banques comme les deux autres chemins.
symbank() { grep "^$1," "$TMP/sym.sym" | cut -d, -f5; }
for pair in gfx0_data:4 gfx1_data:5 gfx2_data:6 gfx3_data:7; do
    n=${pair%:*}; want=${pair#*:}
    [ "$(symbank "$n")" = "$want" ] || {
        echo "ECHEC : $n en banque $(symbank "$n") par les sections, attendu $want"; exit 1; }
done

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
#    `ld bc, __port_ram_gfx0 | __val_gfx0` -> 01 C4 7F, puis 0xC5, 0xC6, 0xC7.
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

# 6. LE MEME CONTROLE, DANS LE SOURCE SEUL. Ce que le n°5 prouve du script,
#    celui-ci le prouve du placement porte par les sections : deplacer une
#    section d'une configuration a l'autre change sa banque ET sa valeur de
#    commutation, sans toucher une adresse logique. C'est la propriete qui fait
#    de `IN` autre chose qu'un `org bN:` deguise.
sed -e 's/SECTION gfx0, "ro" IN w1 OF ext_w1<0>/SECTION gfx0, "ro" IN w1 OF ext_w1<3>/' \
    -e 's/SECTION gfx3, "ro" IN w1 OF ext_w1<3>/SECTION gfx3, "ro" IN w1 OF ext_w1<0>/' \
    "$S" > "$TMP/swapped.asm"
"$FANTAMS" "$TMP/swapped.asm" --target cpc6128 -o "$TMP/symsw.sna" --sym="$TMP/symsw.sym" >/dev/null 2>&1

swsym() { grep "^$1," "$TMP/symsw.sym" | cut -d, -f5; }
[ "$(swsym gfx0_data)" = "7" ] || { echo "ECHEC : gfx0 n'a pas suivi le source (banque $(swsym gfx0_data))"; exit 1; }
[ "$(swsym gfx3_data)" = "4" ] || { echo "ECHEC : gfx3 n'a pas suivi le source (banque $(swsym gfx3_data))"; exit 1; }

cut -d, -f1,4 "$TMP/sym.sym"   | sort > "$TMP/c"
cut -d, -f1,4 "$TMP/symsw.sym" | sort > "$TMP/d"
if ! cmp -s "$TMP/c" "$TMP/d"; then
    echo "ECHEC : deplacer une section dans le source a change une adresse logique"
    diff "$TMP/c" "$TMP/d" || true
    exit 1
fi
if cmp -s "$TMP/sym.sna" "$TMP/symsw.sna"; then
    echo "ECHEC : deplacer une section dans le source n'a rien change aux octets"
    exit 1
fi
echo "acceptation : deplacer une section DANS LE SOURCE change sa banque et sa"
echo "              valeur de commutation, et pas une adresse logique"
