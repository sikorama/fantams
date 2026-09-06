---
status: accepted
---

# Pas de plugins dynamiques : l'extensibilité se paie à la compilation

Nous renonçons explicitement au chargement dynamique de code dans fantams. Un
format de sortie s'ajoute en recompilant, jamais en chargeant un module au
lancement.

## Contexte

L'idée d'enrichir le langage par des directives appelant du code externalisé,
sous forme de plugins, se heurte à la cible réelle du projet : WASM n'a pas de
`dlopen`. Emscripten propose `MAIN_MODULE`/`SIDE_MODULE`, mais au prix d'un
runtime regonflé — précisément ce que nous cherchons à éviter.

Dans cette cible, « plugin » ne peut donc signifier que modularité à la
compilation derrière une interface stable. `sna.h` en donne la forme : fonction
pure, options en structure, sans état ni entrées-sorties.

Quelle interface exactement, et comment les formats se composent, ne relève pas
de cet ADR : c'est l'objet de l'ADR 0007. Aujourd'hui, un seul backend existe —
`sna` — et le choix entre lui et le binaire brut se fait sur l'extension de
`-o`, dans l'adaptateur CLI.

## Conséquences

Si un besoin de véritable extensibilité au runtime apparaît, la réponse ne sera
pas de charger du code dans fantams mais de déplacer la frontière : le cœur rend
une image mémoire et ses métadonnées, et l'empaquetage se fait chez l'hôte. Cette
option reste ouverte et présente l'avantage de rendre structurellement impossible
la contamination de l'assembleur par les formats de sortie.
