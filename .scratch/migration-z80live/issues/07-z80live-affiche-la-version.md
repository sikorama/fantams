# 07: z80live affiche la version de fantams

**What to build:** z80live affiche la version de fantams qu'il fait tourner.
C'est ce qui ferme la boucle du chantier : quand une vérification échouera, la
première question sera « quel fantams ? », et elle se posera **depuis le
navigateur** — c'est là que la péremption a duré trois étages sans être vue.

La chaîne est affichée **telle quelle**. Rien ne l'analyse, aucun ordre entre
versions n'est défini, aucune comparaison n'est faite : un changement de sa forme
ne doit rien casser.

**Blocked by:** 03 (la version en deux dates), 05 (z80live assemble à nouveau).

**Status:** ready-for-agent

- [ ] La version est obtenue à travers l'adaptateur WASM, par la même surface que
      toute autre invocation
- [ ] Elle est visible dans l'interface sans action particulière de l'utilisateur
- [ ] Elle est affichée littéralement ; aucun code ne la découpe ni ne la compare
- [ ] Les deux dates sont lisibles, et l'écart entre elles est apparent
- [ ] Aucun autre travail d'interface n'est entrepris dans ce ticket
