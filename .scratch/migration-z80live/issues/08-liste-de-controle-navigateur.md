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

**Status:** ready-for-human

- [x] La liste est versionnée dans le dépôt consommateur
- [x] Chaque point énonce un geste **et** son résultat attendu — pas « vérifier
      que ça marche »
- [x] Elle couvre au minimum : l'assemblage d'une source, l'affichage de la
      source déroulée, l'affichage d'un diagnostic sur une source fautive, la
      récupération du snapshot, et la version de fantams affichée
- [x] Elle tient en une page et se rejoue sans relire la spec
- [ ] Le mainteneur l'a parcourue une fois de bout en bout, et le résultat est
      consigné

## Commentaires

`VERIFICATION-NAVIGATEUR.md` est versionnée dans le dépôt consommateur. Sept
points, chacun avec **le geste** et **le résultat attendu** — jamais « vérifier
que ça marche » : le point 3 attend une ligne verte et 65 792 octets, le point 5
attend le texte du diagnostic et que le clic place le curseur, le point 6 attend
la signature `MV - SNA` et la taille. Elle tient sur une page et se rejoue sans
relire la spec.

Elle couvre les cinq points exigés : l'assemblage d'une source, la source
déroulée, un diagnostic sur une source fautive, la récupération du snapshot, et
la version de fantams affichée — plus la persistance de la version après
rechargement.

**Le dernier point reste ouvert, et il ne peut pas être coché ici.** Un agent ne
peut pas parcourir la liste : c'est la raison d'être du ticket. Le tableau
« Journal des passages », en fin de document, attend la date, la version
affichée et le résultat. Le ticket ne se ferme qu'une fois cette ligne écrite.
