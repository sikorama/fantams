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

**Status:** resolved

- [x] La liste est versionnée dans le dépôt consommateur
- [x] Chaque point énonce un geste **et** son résultat attendu — pas « vérifier
      que ça marche »
- [x] Elle couvre au minimum : l'assemblage d'une source, l'affichage de la
      source déroulée, l'affichage d'un diagnostic sur une source fautive, la
      récupération du snapshot, et la version de fantams affichée
- [x] Elle tient en une page et se rejoue sans relire la spec
- [x] Le mainteneur l'a parcourue une fois de bout en bout, et le résultat est
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

### Parcourue

Le mainteneur l'a parcourue de bout en bout le 2026-09-08, **7 points sur 7**,
sur une source réelle portant macro, boucle `for`, `org`, `run` et `equ`. Le
journal de passage est rempli dans le document lui-même.

Deux remarques en sont sorties :

- **le point 6 est plus faible que ce que la machine a déjà prouvé.** Le
  mainteneur l'a noté : l'émulateur autodétecte le conteneur par ses octets
  magiques, donc un snapshot qu'il accepte *et exécute* a nécessairement un
  en-tête recevable. Le `head -c 8` de la liste est le contrôle du pauvre à côté
  de l'épreuve du ticket 06 ;
- **la source déroulée perd les lignes vides et les commentaires**, ce qui la
  rend « assez brute à relire ». C'est une incohérence réelle entre ce que
  `asm_main.cpp` annonce pour `-E` et ce que l'ADR 0013 promet ; hors périmètre,
  consignée en ticket 09.
