---
status: accepted
---

# Une valeur matérielle vit dans un dossier de recherche ou dans un profil, jamais dans la spec

Les chiffres du matériel — champs de bits, ports, polarités, tables de
configuration — n'ont que deux domiciles : `docs/recherche/*.md` pour les
valeurs *vérifiées et citées*, et les profils de cible pour les valeurs
*exécutables*. Aucun autre document n'en porte, la spécification de la chaîne
d'outils comprise.

## Contexte

La spécification soutenait déjà qu'une valeur matérielle doit être vérifiée en un
seul endroit — le profil — et recopiait pourtant la carte mémoire du CPC dans son
propre corps, à titre d'illustration. La vérification sur sources primaires a
trouvé deux fautes, précisément là :

- la **polarité des bits de ROM** du registre `RMR` était inversée : 0 active,
  1 inhibe, et sept sources indépendantes de quatre niveaux d'autorité le
  disent, sans exception ;
- la configuration `ccc = 000` y était glosée « aucune RAM étendue connectée »
  alors que c'est la disposition **linéaire par défaut**, extension présente ou
  non. Le symbole `__cfg_none` du même document portait l'erreur jusque dans son
  nom.

Le paragraphe qui plaidait pour l'unicité de la source a donc démontré sa thèse
à ses dépens. **Une valeur écrite deux fois est une valeur fausse une fois.**

Second constat, du même travail : l'exemple choisi pour justifier l'unicité — la
polarité des bits de ROM, présentée comme « précisément le genre d'information
sur laquelle les sources se contredisent » — était le seul point où la
littérature est unanime. Le piège y est de nommage (un registre « ROM *enable* »
dont les bits s'appellent « ROM *disable* »), non de valeur.

## Décision

La spécification n'énumère plus que les **natures** d'information qu'un profil
doit savoir dire, avec pour chacune le mot du vocabulaire qui la porte. Les
valeurs restent dans les dossiers de recherche, avec leurs citations, leur niveau
d'autorité et leurs contradictions non levées signalées comme telles.

Les exemples qui justifient l'unicité sont désormais des contradictions réelles
et documentées : l'effet du bit 4 de `RMR`, le décodage du port du PAL, le nombre
de bits de page réellement décodés.

## Conséquences

- Un tableau de la spec devient une liste de contrôle : pour chaque nature, le
  vocabulaire a-t-il un mot ? C'est le test d'acceptation du modèle, et il se
  relit sans linker.
- Un dossier de recherche devient un livrable de premier plan, pas une note de
  travail : c'est lui qui répond de l'exactitude.
- Vérification documentaire et vérification sur machine ne sont plus confondues.
  Une **source de vérification** versionnée, une par valeur litigieuse, dont le
  juge est la machine, comble l'écart que « vérifiée une fois, sur machine ou sur
  émulateur » laissait ouvert.
