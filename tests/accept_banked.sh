#!/bin/sh
# accept_banked.sh — le critere de fin de l'etage C1.
#
# L'exemple du §12.2, sans sa section compressee (D9) : une source SANS UN SEUL
# `org`, une carte de dix lignes, et quatre controles. Le quatrieme est celui qui
# compte — c'est lui qui prouve que le renversement du §12.2 a eu lieu.
set -e
cd "$(dirname "$0")/.."
FANTAMS=${FANTAMS:-./fantams}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

A=examples/banked.asm
L=examples/banked.ld

# La source ne nomme AUCUN emplacement : c'est l'enonce, et il se verifie.
if grep -nE '^[^;]*\b(org|BANK)\b' "$A" >/dev/null; then
    echo "ECHEC : la source d'acceptation nomme un emplacement"
    exit 1
fi

"$FANTAMS" "$A" -T "$L" -o "$TMP/g.sna" --sym="$TMP/g.sym" >/dev/null 2>&1

sym() { grep "^$1," "$TMP/g.sym" | cut -d, -f4; }
bank() { grep "^$1," "$TMP/g.sym" | cut -d, -f5; }

# 1. Chaque section est rangee dans la banque que sa configuration dicte.
for pair in start:1 sysbank_audio_init:2 audio_init:5 unpacked:3; do
    n=${pair%:*}; want=${pair#*:}
    got=$(bank "$n")
    if [ "$got" != "$want" ]; then
        echo "ECHEC : $n est en banque $got, attendu $want"
        exit 1
    fi
done

# 2. `audio_init` vaut l'adresse de la FENETRE, sans qu'un `org` l'ait dit.
if [ "$(sym audio_init)" != "0x4000" ]; then
    echo "ECHEC : audio_init vaut $(sym audio_init), attendu 0x4000"
    exit 1
fi
# Et `main`, dans une autre banque, vaut la meme adresse logique : c'est
# exactement ce qu'aucune derivation par l'adresse ne saurait faire.
if [ "$(sym start)" != "0x4000" ]; then
    echo "ECHEC : start vaut $(sym start), attendu 0x4000"
    exit 1
fi

# 3. Les valeurs de commutation du §12.3, dans les octets emis.
#    `ld bc, __port_ram_audio | __val_ram_audio` -> 01 C5 7F
#    `ld bc, __port_ram_linear | __val_ram_linear` -> 01 C0 7F
#    Les deux sont dans `sysbank`, rangee en banque 2 : offset 256 + 2*0x4000.
head=$(od -An -tx1 -j $((256 + 2 * 16384)) -N 8 "$TMP/g.sna" | tr -d ' \n')
case "$head" in
    01c57fed49cd0040) ;;
    *) echo "ECHEC : sysbank commence par $head"
       echo "        attendu 01c57fed49cd0040 — ld bc,&7FC5 / out (c),c / call &4000"
       exit 1 ;;
esac

# 4. LE CONTROLE QUI COMPTE. Deplacer `audio` d'une banque a l'autre DANS LE
#    SCRIPT SEUL doit changer sa banque de rangement, et PAS UNE adresse logique.
sed 's/ext_w1<1>/ext_w1<2>/' "$L" > "$TMP/moved.ld"
"$FANTAMS" "$A" -T "$TMP/moved.ld" -o "$TMP/m.sna" --sym="$TMP/m.sym" >/dev/null 2>&1

if [ "$(grep '^audio_init,' "$TMP/m.sym" | cut -d, -f5)" != "6" ]; then
    echo "ECHEC : deplacer la section n'a pas change sa banque"
    exit 1
fi
# Les adresses logiques, colonne par colonne : identiques.
#
# Des fichiers temporaires et non une substitution de processus : ce script est
# lance par les DEUX chaines de test, et l'une d'elles emploie un /bin/sh qui
# n'est pas bash. Un test qui ne passe que sur une des deux listes ne surveille
# rien sur l'autre.
cut -d, -f1,4 "$TMP/g.sym" > "$TMP/a"
cut -d, -f1,4 "$TMP/m.sym" > "$TMP/b"
if ! cmp -s "$TMP/a" "$TMP/b"; then
    echo "ECHEC : deplacer la section a change une adresse logique"
    diff "$TMP/a" "$TMP/b" || true
    exit 1
fi
# Et la valeur de commutation a suivi : &C6 au lieu de &C5.
head=$(od -An -tx1 -j $((256 + 2 * 16384)) -N 3 "$TMP/m.sna" | tr -d ' \n')
if [ "$head" != "01c67f" ]; then
    echo "ECHEC : la valeur de commutation n'a pas suivi la section ($head)"
    exit 1
fi

echo "acceptation : le §12.2 sans compression — 5 sections, 4 banques, 0 org ;"
echo "              deplacer une section dans le script change sa banque et sa"
echo "              valeur de commutation, et pas une adresse logique"
