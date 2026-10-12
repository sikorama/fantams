---
status: accepted
---

# fantams définit son langage, et le corpus ne mesure que ce qui compile en l'état

fantams décide seul de son langage : une décision se justifie par la cohérence du
langage, par les octets produits et par la sûreté, jamais par imitation. Aucun document,
aucun commentaire ni aucun test du dépôt ne définit un terme ou une règle par
contraste avec un autre outil.

Le corpus de sources externes ne mesure qu'une chose : **une source compile-t-elle
en l'état avec fantams ?** S'il y a un écart, c'est la source qu'on corrige — par le
développeur de la source, pas par un agent — et non le langage. Le corpus n'est ni
un étalon d'octets ni un objectif de compatibilité.

## Contexte

Un terme défini par ce qu'il n'imite pas est un terme qu'on ne peut pas énoncer
seul : le glossaire définissait le corpus, le portage et le module par contraste,
et plusieurs ADR mesuraient leur décision contre une sortie extérieure plutôt que
contre leurs propres raisons.

L'écart de conception est par ailleurs devenu tel que la comparaison n'informe
plus : le langage, le préprocesseur, le modèle mémoire et le découpage
assembleur / linker ne se lisent pas comme des variantes de l'existant.

## Décision

Retrait de toute mention, code compris. Les termes du glossaire s'énoncent en
propre. Les chapitres qui s'appuyaient sur un contre-exemple gardent l'évidence
— les directives nommables qui franchissent le périmètre d'un assembleur — et
perdent l'attribution : c'est l'évidence qui portait la démonstration.

Contrainte tenue pendant la réécriture des ADR : **aucun ne doit perdre sa
raison.** Un ADR devenu inintelligible sans le nom du tiers aurait été un ADR
justifié par l'imitation, et l'apprendre valait la relecture.

Les mots réservés `BUILDSNA`, `BANKSET`, `NOLIST`, `LIST` (acceptés et ignorés) et
`BANK`, `SNASET`, `SETCPC`, `CHARSET`, `TICKER`, `STR` (refusés nommément)
**restent réservés** : ce sont des directives de format, de machine hôte ou de
pagination, que le périmètre d'un assembleur exclut, et échouer nommément vaut
mieux que les lire comme un label.

## Conséquences

- Le corpus mesure ce qui compile en l'état, et ne dicte pas ce qui est correct.
  Les cas de référence versionnés restent la seule autorité sur la sortie attendue.
- Il n'y a plus de comparaison octet à octet avec la sortie extérieure : un
  écart de sortie sur une source du corpus n'est pas un défaut de fantams.
- `BANK` reste refusé dans une source alors que le mot devient central dans le
  vocabulaire des profils de cible. Ce n'est pas une contradiction, c'est la
  frontière du découpage : le profil décrit la machine, la source n'en parle pas.
- Le coût d'adapter une source devient une information neutre, mesurée en lignes
  éditées, plutôt qu'une dette.
