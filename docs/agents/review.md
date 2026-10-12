# Revue de code

Règles pour l'agent qui relit un diff de ce dépôt (standards ou spec).

- **Un constat de comportement se reproduit.** Avant d'écrire « X passe sans
  erreur » ou « X n'est pas refusé », construire `ppdump` (`make ppdump`), lancer
  un source minimal et citer la commande avec sa sortie. Un constat sans cette
  trace est une hypothèse : le dire.
- **Les verrous sautés se rapportent.** `make test` peut finir à 0 en ayant
  sauté le verrou natif == WASM ou l'épreuve du snapshot ; la sortie le
  signale. Le dire dans la revue, ne pas écrire « les tests passent » seul.
- **Boucle courte** : `make test-fast` (tests unitaires seuls) ; `make test`
  dépasse deux minutes.
- **Format** : `scripts/format.sh` contrôle les lignes modifiées, `make format`
  les réécrit. Ne pas relever un écart de format que ce script règle.
