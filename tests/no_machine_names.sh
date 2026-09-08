#!/bin/sh
# no_machine_names.sh — l'invariant mécanique du §13.2, test d'acceptation n°3
# du modèle : **aucun nom de machine dans le code du linker.**
#
# « Décrire une machine ne doit pas demander de toucher au code du linker » est
# la seule affirmation du §13 qui puisse se vérifier. Ce script la vérifie.
#
# Il est inscrit AVEC le code qu'il surveille, et non après, parce qu'un
# invariant écrit après le code est un invariant qu'on affaiblit pour le faire
# passer (§13.2).
#
# LE PÉRIMÈTRE EST NOMMÉ, et c'est ce qui distingue cet invariant d'un grep
# décoratif :
#
#   - il porte sur le LINKER et sur les deux langages qu'il lit ;
#   - `profiles.cpp` est EXEMPT : c'est une donnée, pas du code. Il est le seul,
#     et c'est la décision D1 — un porteur unique pour les valeurs d'une machine ;
#   - le BUILDER en est dehors : un `.SNA` est un format d'une machine précise,
#     et son backend a toutes les raisons de la connaître ;
#   - les TESTS en sont dehors : leur travail est justement de nommer des
#     machines et de vérifier qu'il ne se passe rien de particulier.
set -e
cd "$(dirname "$0")/.."

SOURCES="link.cpp link.h script.cpp script.h profile.cpp profile.h lex.cpp lex.h"
NAMES='cpc|amstrad|zx|spectrum|msx|konami|amsdos|sinclair|gate.array|crtc|rmr'

hits=$(grep -niE "$NAMES" $SOURCES || true)
if [ -n "$hits" ]; then
    echo "ECHEC : un nom de machine est entre dans le code du linker."
    echo "        Ce qu'il decrit appartient a un PROFIL, qui est une donnee."
    echo "$hits"
    exit 1
fi
echo "acceptation : aucun nom de machine dans le linker ($(echo $SOURCES | wc -w) fichiers)"
