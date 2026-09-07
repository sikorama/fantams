---
status: accepted
---

# La compatibilité avec un autre assembleur n'est pas un objectif

fantams définit son langage. Le corpus de sources externes reste un instrument
de mesure de la **diversité syntaxique** rencontrée dans la nature, et donc du
travail d'implémentation restant ; il n'est plus un objectif de compatibilité.
Aucun document ni commentaire du dépôt ne se définit par contraste avec un outil
tiers.

## Contexte

Le projet est né à côté d'un assembleur existant, et son vocabulaire en gardait
la trace : 151 occurrences d'un nom d'outil tiers dans 37 fichiers, dont le nom
du projet CMake, quatre entrées de `CONTEXT.md`, quinze ADR sur vingt et un, et
la charnière argumentative de deux chapitres de la spécification.

Trois de ces emplois n'étaient pas de la citation mais de la **définition par
contraste** : le glossaire définissait le corpus comme « les sources écrites
pour » cet outil, le portage comme « le travail pour qu'une telle source
assemble », et le module comme « une divergence assumée vis-à-vis » de lui. Un
terme défini par ce qu'il n'imite pas est un terme qu'on ne peut pas énoncer
seul.

L'écart de conception est par ailleurs devenu tel que la comparaison n'informe
plus : le langage, le préprocesseur, le modèle mémoire et le découpage
assembleur / linker ne se lisent pas comme des variantes de l'existant.

## Décision

Retrait de toute mention, code compris. Les termes du glossaire s'énoncent en
propre. Les chapitres qui s'appuyaient sur un contre-exemple gardent l'évidence
— les directives nommables qui franchissent le périmètre d'un assembleur — et
perdent l'attribution : c'est l'évidence qui portait la démonstration.

Les ADR antérieurs mesuraient souvent une décision contre la sortie de cet
outil, et cette mesure *est* leur raison : elle ne disparaît pas avec le nom. Ils
disent désormais **« l'assembleur de référence »** — l'oracle externe employé à
l'époque pour comparer des octets. Le fait est conservé, l'allégeance non.

Contrainte tenue pendant la réécriture des ADR : **aucun ne doit perdre sa
raison.** Un ADR devenu inintelligible sans le nom du tiers aurait été un ADR
justifié par l'imitation, et l'apprendre valait la relecture.

Les mots réservés qui n'existaient que par courtoisie envers cet outil —
`BUILDSNA`, `BANKSET`, `NOLIST`, `LIST` acceptés et ignorés ; `BANK`, `SNASET`,
`SETCPC`, `CHARSET`, `TICKER`, `STR` refusés nommément — **restent réservés**,
avec une autre justification : ce sont des directives de format, de machine hôte
ou de pagination, que le périmètre d'un assembleur exclut, et échouer nommément
vaut mieux que les lire comme un label.

## Conséquences

- Le corpus perd son statut d'étalon et garde son usage : il mesure ce qui reste
  à implémenter, il ne dicte pas ce qui est correct. Les cas de référence
  versionnés restent la seule autorité sur la sortie attendue.
- `BANK` reste refusé dans une source alors que le mot devient central dans le
  vocabulaire des profils de cible. Ce n'est pas une contradiction, c'est la
  frontière du découpage : le profil décrit la machine, la source n'en parle pas.
- Le coût d'un portage devient une information neutre, mesurée en lignes
  éditées, plutôt qu'une dette.
