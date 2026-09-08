#!/usr/bin/env bash
# epreuve_snapshot.sh — la premiere EPREUVE du projet.
#
# Une epreuve n'est pas un test d'octets : c'est l'execution d'un artefact sur
# une machine, pour constater qu'elle l'ACCEPTE et ce qu'elle en fait. Son
# autorite porte sur la RECEVABILITE, jamais sur les octets — un octet se teste
# depuis une image fabriquee a la main, sans machine, et cette regle reste
# entiere. C'est un instrument de mesure, de la famille du corpus et de la
# machine reelle.
#
# Le cycle de vie reprend, invariant pour invariant, celui du test de fumee du
# frontal sans affichage de l'emulateur : configuration jetable, attente du
# demarrage par SONDAGE, verification que le compteur de trames avance, arret
# par la voie ordonnee avec controle du code de sortie.
#
# Le chargement passe par l'endpoint UNIQUE qui accepte tous les conteneurs, le
# format etant autodetecte par ses octets magiques. Pour ce chantier, un seul
# artefact est eprouve : le snapshot.
#
# AUCUN POINT D'ARRET n'est pose, et c'est delibere : la notation physique de
# l'emulateur est un faux ami de la notre — graphie identique, semantique
# differente (banque en hexa contre decimal, offset qui REPORTE au-dela de 16 K
# contre adresse logique MASQUEE). Tant que l'epreuve n'en a pas besoin, elle
# n'ecrit pas de traducteur ; le jour ou elle en posera un, la traduction vivra
# a un seul endroit, nommee pour ce qu'elle est.
#
# Dependances, et discipline de saut :
#   AMSPIRIT_HEADLESS  le frontal sans affichage. Absent -> le test se SAUTE.
#   AMSPIRIT_ROMS      le jeu de ROM qu'il exige (-R). Absent -> le test se SAUTE.
#   EPREUVE_PORT       le port du serveur (defaut 8791, distinct de celui du
#                      test de fumee et de celui d'une instance de travail).
# Construire fantams n'impose pas un second depot : c'est pourquoi l'absence se
# saute au lieu d'echouer.
set -u
cd "$(dirname "$0")/.."
ROOT=$(pwd)
FANTAMS=${FANTAMS:-./fantams}
PORT=${EPREUVE_PORT:-8791}
BASE="http://127.0.0.1:$PORT"

skip() { echo "SAUTE : $1 — l'epreuve du snapshot n'a PAS eu lieu"; exit 77; }

BIN=${AMSPIRIT_HEADLESS:-}
[ -n "$BIN" ] && [ -x "$BIN" ] || skip "frontal sans affichage absent (AMSPIRIT_HEADLESS)"
ROMS=${AMSPIRIT_ROMS:-}
[ -n "$ROMS" ] && [ -d "$ROMS" ] || skip "jeu de ROM absent (AMSPIRIT_ROMS)"
command -v curl >/dev/null 2>&1 || skip "curl absent"
command -v python3 >/dev/null 2>&1 || skip "python3 absent"

# Un serveur repond deja sur ce port : c'est l'instance de QUELQU'UN. On ne
# pilote pas une machine qu'on n'a pas demarree — et surtout on ne lui envoie
# pas /api/quit.
if curl -sf --max-time 1 "$BASE/api/ping" >/dev/null 2>&1; then
    skip "le port $PORT est deja servi par une autre instance (EPREUVE_PORT pour en changer)"
fi

WORK=$(mktemp -d)
LOG="$WORK/emu.log"
PID=""
cleanup() {
    [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null && kill "$PID" 2>/dev/null
    rm -rf "$WORK"
}
trap cleanup EXIT

fail() {
    echo "ECHEC : $1"
    echo "--- dernieres lignes du journal de l'emulateur ---"
    tail -20 "$LOG" 2>/dev/null
    exit 1
}

frames_of() { python3 -c 'import json,sys; print(json.loads(sys.argv[1])["emu"]["frames"])' "$1"; }

# --- 1. L'artefact -------------------------------------------------------
# Le cas de reference est ASSEMBLE ici, par l'adaptateur qu'on eprouve.
SNA="$WORK/marqueur.sna"
"$FANTAMS" tests/epreuve/marqueur.asm -o "$SNA" >/dev/null 2>&1 \
    || fail "l'assemblage du cas de reference a echoue"
[ -s "$SNA" ] || fail "aucun snapshot produit"

# L'adresse et la valeur que le cas de reference a ete ECRIT pour produire.
# Little-endian : 0x5A, puis 0xCAFE ecrit par « ld (MARQUEUR+1),hl ».
MARQUEUR_ADDR=0x9000
MARQUEUR_LEN=3
ATTENDU=5afeca

# Avant l'execution, ces trois octets valent zero DANS LE SNAPSHOT. C'est ce qui
# fait que les relire non nuls prouve une execution, et pas seulement un
# chargement. On le constate plutot que de l'affirmer.
avant=$(python3 -c '
import sys
d = open(sys.argv[1], "rb").read()
print(d[256 + 0x9000:256 + 0x9003].hex())' "$SNA")
[ "$avant" = "000000" ] || fail "le cas de reference porte deja $avant a $MARQUEUR_ADDR : il ne prouverait plus rien"

# --- 2. La machine, avec une configuration jetable ------------------------
"$BIN" -C "$WORK/cfg" -R "$ROMS" --web-port "$PORT" > "$LOG" 2>&1 &
PID=$!

# --- 3. Le demarrage, par sondage ----------------------------------------
PING=""
for _ in $(seq 1 50); do
    PING=$(curl -sf --max-time 2 "$BASE/api/ping" 2>/dev/null) && break
    kill -0 "$PID" 2>/dev/null || fail "l'emulateur est mort pendant son demarrage"
    sleep 0.2
done
[ -n "$PING" ] || fail "/api/ping n'a jamais repondu"

# --- 4. L'emulation avance-t-elle vraiment ? ------------------------------
# Un emulateur fige repondrait /api/ping sans emuler : il ne doit pas passer
# pour un succes.
F1=$(frames_of "$PING") || fail "/api/ping illisible : $PING"
F2="$F1"
for _ in $(seq 1 40); do
    sleep 0.05
    F2=$(frames_of "$(curl -sf "$BASE/api/ping")") || fail "second /api/ping en echec"
    [ "$F2" -gt "$F1" ] && break
done
[ "$F2" -gt "$F1" ] || fail "le compteur de trames n'avance pas ($F1 -> $F2) : emulateur fige"

# --- 5. Poster l'artefact ------------------------------------------------
# L'endpoint UNIQUE qui accepte tous les conteneurs ; le format est autodetecte
# par les octets magiques, ici « MV - SNA ».
rep=$(curl -sf -X POST --data-binary "@$SNA" "$BASE/api/media?name=marqueur.sna" 2>&1) \
    || fail "POST /api/media a echoue : $rep"
echo "$rep" | grep -q '"ok"[[:space:]]*:[[:space:]]*true' \
    || fail "l'emulateur a refuse l'artefact : $rep"

# --- 6. Laisser tourner --------------------------------------------------
F3=$(frames_of "$(curl -sf "$BASE/api/ping")") || fail "/api/ping apres chargement"
F4="$F3"
for _ in $(seq 1 60); do
    sleep 0.05
    F4=$(frames_of "$(curl -sf "$BASE/api/ping")") || fail "/api/ping pendant l'execution"
    [ "$F4" -gt "$F3" ] && break
done
[ "$F4" -gt "$F3" ] || fail "l'emulation n'avance plus apres le chargement ($F3 -> $F4)"

# --- 7. Constater l'effet ------------------------------------------------
# Une LECTURE D'ETAT, sur l'adresse que le cas de reference a ete ecrit pour
# ecrire. `view=cpu` : ce que le Z80 voit reellement, overlays ROM appliques —
# c'est ce que le programme a ecrit, pas une banque choisie par le harnais.
lu=""
for _ in $(seq 1 40); do
    rep=$(curl -sf "$BASE/api/ram?addr=$MARQUEUR_ADDR&len=$MARQUEUR_LEN&view=cpu") || rep=""
    [ -n "$rep" ] && lu=$(python3 -c '
import json,sys
print(json.loads(sys.argv[1])["hex"].replace(" ", "").lower())' "$rep" 2>/dev/null)
    [ "${lu:-}" = "$ATTENDU" ] && break
    sleep 0.1
done
if [ "${lu:-}" = "$ATTENDU" ]; then
    echo "epreuve : la machine a accepte le snapshot et l'a execute — $MARQUEUR_ADDR = $lu"
else
    fail "a $MARQUEUR_ADDR : attendu $ATTENDU, lu ${lu:-<rien>} (avant execution : $avant)"
fi

# --- 8. L'arret, par la voie ordonnee ------------------------------------
curl -sf -X POST "$BASE/api/quit" >/dev/null || fail "POST /api/quit a echoue"
for _ in $(seq 1 25); do
    kill -0 "$PID" 2>/dev/null || break
    sleep 0.2
done
kill -0 "$PID" 2>/dev/null && fail "l'emulateur vit encore apres /api/quit"
wait "$PID"; RC=$?
PID=""
[ "$RC" -eq 0 ] || fail "l'emulateur est sorti avec $RC apres /api/quit"

echo "epreuve : arret ordonne, code de sortie 0 ($F1 -> $F4 trames)"
