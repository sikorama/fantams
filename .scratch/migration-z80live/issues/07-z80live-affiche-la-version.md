# 07: z80live affiche la version de fantams

**What to build:** z80live affiche la version de fantams qu'il fait tourner.
C'est ce qui ferme la boucle du chantier : quand une vérification échouera, la
première question sera « quel fantams ? », et elle se posera **depuis le
navigateur** — c'est là que la péremption a duré trois étages sans être vue.

La chaîne est affichée **telle quelle**. Rien ne l'analyse, aucun ordre entre
versions n'est défini, aucune comparaison n'est faite : un changement de sa forme
ne doit rien casser.

**Blocked by:** 03 (la version en deux dates), 05 (z80live assemble à nouveau).

**Status:** resolved

- [x] La version est obtenue à travers l'adaptateur WASM, par la même surface que
      toute autre invocation
- [x] Elle est visible dans l'interface sans action particulière de l'utilisateur
- [x] Elle est affichée littéralement ; aucun code ne la découpe ni ne la compare
- [x] Les deux dates sont lisibles, et l'écart entre elles est apparent
- [x] Aucun autre travail d'interface n'est entrepris dans ce ticket

## Commentaires

Résolu côté dépôt consommateur (commit « version de fantams affichee, et la
liste de controle navigateur »).

- `wasm/assemble.mjs` gagne `fantamsVersion(factories)` : un `callMain
  (['--version'])`, par la même surface que toute autre invocation. L'adaptateur
  n'exporte rien de plus pour la servir ;
- `app/src/App.svelte` l'appelle au montage, sans action de l'utilisateur, et
  l'affiche dans l'en-tête à droite de `z80live` ;
- elle est affichée **littéralement** : la chaîne rendue est posée telle quelle
  dans le DOM. Aucun code ne la découpe, ne la compare, ni ne définit d'ordre
  entre versions. Le seul motif du dépôt qui la regarde est dans le *test*, et
  il ne vérifie que la forme ;
- les deux dates sont lisibles et l'écart entre elles apparent ; l'infobulle
  dit ce que l'écart signifie ;
- aucun autre travail d'interface n'a été entrepris.

Le cas 8 de `npm run test:wasm` la vérifie au niveau node : `[OK] version —
fantams 2026-09-08 (compile 2026-09-08)`. Qu'elle soit **visible dans le
navigateur** est le point 1 de la liste de contrôle du ticket 08 — un agent ne
peut pas le constater.
