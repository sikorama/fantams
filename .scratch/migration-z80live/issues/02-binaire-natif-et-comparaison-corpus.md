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

**Status:** ready-for-agent

- [ ] Le binaire natif est constructible dans le répertoire du sous-module, par
      une commande documentée
- [ ] La comparaison sur le corpus s'exécute jusqu'au bout et produit son rapport
- [ ] Le rapport n'est pas écrit dans un chemin suivi par le contrôle de version
      (cf. ADR 0029)
- [ ] Le nombre de sources traitées, et la répartition entre concordances,
      échecs d'assemblage et divergences, sont constatés et rapportés au
      mainteneur — sans en tirer de conclusion : la distinction entre « fantams
      ne sait pas faire » et « la source demande un portage » est hors périmètre
