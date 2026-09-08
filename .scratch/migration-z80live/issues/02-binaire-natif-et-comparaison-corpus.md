# 02: Le binaire natif du sous-module, et la comparaison sur corpus en marche

**What to build:** la comparaison sur le corpus retourne en état de marche. Elle
assemble chaque source du corpus avec l'assembleur de référence et avec fantams,
puis compare les images mémoire octet à octet — et elle est aujourd'hui à
l'arrêt parce que le binaire natif de fantams n'a jamais été construit à
l'endroit où elle l'attend, dans le répertoire du sous-module.

C'est la **seconde panne** du chantier, et elle est indépendante de celle du
WASM : la liste de sources du Makefile est complète, personne n'avait simplement
lancé la construction là.

Ce ticket ne corrige aucune divergence trouvée par la comparaison. Il remet
l'instrument en marche ; l'exploitation de ce qu'il mesure est le chantier
suivant.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Le binaire natif est constructible dans le répertoire du sous-module, par
      une commande documentée
- [x] La comparaison sur le corpus s'exécute jusqu'au bout et produit son rapport
- [x] Le rapport n'est pas écrit dans un chemin suivi par le contrôle de version
      (cf. ADR 0029)
- [x] Le nombre de sources traitées, et la répartition entre concordances,
      échecs d'assemblage et divergences, sont constatés et rapportés au
      mainteneur — sans en tirer de conclusion : la distinction entre « fantams
      ne sait pas faire » et « la source demande un portage » est hors périmètre

## Commentaires

Résolu, côté dépôt consommateur (commit « compare: le binaire natif se
construit, et son absence se dit »).

- `npm run build:native` construit `fantams/fantams` — `make -C fantams
  fantams`, donc le manifeste du ticket 01, donc une seule liste ;
- l'absence du binaire arrêtait la comparaison sur un `ENOENT` d'`execFileSync`
  qui ne nommait ni la cause ni le remède. Elle s'arrête maintenant en nommant
  les deux. C'est comme cela qu'un instrument reste à l'arrêt sans qu'on sache
  pourquoi ;
- `QUICKSTART.md` documente la commande et la comparaison ;
- le rapport détaillé reste dans un chemin **exclu** du suivi de version — la
  ligne de `.gitignore` existait déjà, et l'ADR 0029 dit qu'elle est l'effet
  dont il est la cause.

### Ce que la mesure a constaté

La comparaison s'exécute de bout en bout avec un fantams à jour. Sur **179
sources comparables** (le corpus filtré : celles qui assemblent avec la
référence et produisent un snapshot) :

| | brut | textes distincts |
|---|---|---|
| concordances | 89 (49,7 %) | 84 (48,6 %) |
| divergences | 16 (8,9 %) | 16 (9,2 %) |
| échecs d'assemblage | 74 (41,3 %) | 73 (42,2 %) |

Les échecs se répartissent sur **dix-neuf causes distinctes**, dont les cinq
premières couvrent un peu plus de la moitié : `BANK` non supporté (10), deux
notations à préfixe de format refusées (10 et 5), un mot réservé employé comme
index de boucle (10), et des symboles inconnus (6).

**Aucune conclusion n'est tirée ici.** Distinguer « fantams ne sait pas faire »
de « la source demande un portage », classer les constructions par diversité
syntaxique et proposer des sources au mainteneur est le chantier suivant, et la
spec le met hors périmètre. Ce ticket remet l'instrument en marche, rien de
plus.

Les identifiants et les noms de sources ne sont pas recopiés ici : ils vivent
dans le rapport non versionné (ADR 0029).
