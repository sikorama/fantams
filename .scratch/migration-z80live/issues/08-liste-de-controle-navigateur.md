# 08: La liste de contrôle navigateur

**What to build:** la procédure écrite des vérifications que **seul un humain**
peut faire — celles qu'aucun test de la suite ne remplace : le navigateur,
l'éditeur, les diagnostics affichés, le snapshot téléchargé, la version visible.

Ce ticket existe séparément **parce qu'il est maigre**. C'est la seule étape du
chantier qu'un agent ne peut pas exécuter, et donc la seule qu'on peut déclarer
faite sans l'avoir faite. Une liste écrite rend cela impossible.

Elle est courte et ordonnée : chaque point énonce le geste et le résultat
attendu. Elle vit dans le dépôt consommateur, versionnée, pour être rejouable au
prochain chantier.

**Blocked by:** 07 (z80live affiche la version de fantams).

**Status:** ready-for-agent

- [ ] La liste est versionnée dans le dépôt consommateur
- [ ] Chaque point énonce un geste **et** son résultat attendu — pas « vérifier
      que ça marche »
- [ ] Elle couvre au minimum : l'assemblage d'une source, l'affichage de la
      source déroulée, l'affichage d'un diagnostic sur une source fautive, la
      récupération du snapshot, et la version de fantams affichée
- [ ] Elle tient en une page et se rejoue sans relire la spec
- [ ] Le mainteneur l'a parcourue une fois de bout en bout, et le résultat est
      consigné
