---
status: accepted
---

# Aucune directive de format de sortie dans le source

Le format produit et ses paramètres — modèle de CPC, contenu de l'en-tête du
snapshot, géométrie d'une disquette — sont fixés à l'invocation, jamais par une
directive écrite dans le source. `SNASET` et `SETCPC` sont **refusés**, par un
message qui nomme le remplaçant : passer l'information à l'invocation.

## Contexte

Une source décrit un programme. Le format dans lequel on l'empaquette décrit une
livraison. Mélanger les deux rend une source non réutilisable d'un contexte à
l'autre : la même routine ne peut pas partir en snapshot pour l'émulateur du
navigateur et en piste de disquette pour une compilation, alors que le code, lui,
est identique.

C'est aussi la porte par laquelle les assembleurs dérivent. Une fois qu'une
directive peut décrire la sortie, plus rien ne délimite ce qu'une directive peut
décrire — et une chaîne complète finit par embarquer l'export audio et les
segments compressés
par ce chemin.

## Conséquences

Le cœur rend une image mémoire et ses métadonnées ; le choix du backend et ses
options appartiennent à l'adaptateur, donc à la ligne de commande, à l'appel
d'API ou à la configuration de l'hôte. C'est l'application directe de la règle de
tri de l'ADR 0001 et le premier cas concret qui la met à l'épreuve.

En contrepartie, une source du corpus qui s'appuyait sur `SNASET` pour se décrire
elle-même perd cette autonomie : l'information doit être transmise au moment de
l'invocation. Le refus la porte au grand jour plutôt que de laisser croire, par
un simple avertissement, que la directive a quand même agi — un `SNASET` ignoré
en silence produirait un snapshot dont l'en-tête n'est pas celui que le source
annonce.

Cette décision ne couvre pas `ORG` ni le placement en banque, qui décrivent où le
programme s'exécute et relèvent donc bien du source.
