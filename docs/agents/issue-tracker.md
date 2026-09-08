# Issue tracker : markdown local

Les issues et les specs de ce dépôt sont des fichiers markdown sous `.scratch/`.
Il n'y a pas de tracker externe : les issues GitHub ne sont pas utilisées.

## Conventions

- **Un chantier, un répertoire** : `.scratch/<chantier>/`
- La spec est `.scratch/<chantier>/spec.md`
- Les tickets d'implémentation sont **un fichier par ticket**, à
  `.scratch/<chantier>/issues/NN-<slug>.md`, numérotés depuis `01` — jamais un
  fichier unique regroupant tous les tickets
- L'état de triage est une ligne `Status:` en tête du fichier (les chaînes de
  rôle sont dans `triage-labels.md`)
- Les arêtes bloquantes sont une ligne `Blocked by: NN, NN` en tête. Un ticket
  est débloqué quand tous les fichiers qu'il cite sont `resolved`
- Les commentaires et l'historique de conversation s'ajoutent en bas du fichier,
  sous un titre `## Commentaires`

## « Publier sur le tracker »

Créer un fichier sous `.scratch/<chantier>/`, en créant le répertoire au besoin.

## « Récupérer le ticket concerné »

Lire le fichier au chemin indiqué. Le chemin ou le numéro d'issue est
normalement fourni directement.

## Opérations de cartographie

Employées par `/wayfinder`. La **carte** est un fichier, avec un fichier
**enfant** par ticket.

- **Carte** : `.scratch/<chantier>/map.md` (le corps Notes / Décisions acquises
  / Brouillard)
- **Ticket enfant** : `.scratch/<chantier>/issues/NN-<slug>.md`, numéroté depuis
  `01`, la question dans le corps. Une ligne `Type:` porte le type de ticket
  (`research` / `prototype` / `grilling` / `task`) ; une ligne `Status:` porte
  `claimed` ou `resolved`
- **Blocage** : la ligne `Blocked by: NN, NN` en tête
- **Frontière** : parcourir `.scratch/<chantier>/issues/` et retenir les
  fichiers ouverts, débloqués et non réservés ; le plus petit numéro l'emporte
- **Réserver** : mettre `Status: claimed` et enregistrer **avant** tout travail
- **Résoudre** : ajouter la réponse sous un titre `## Réponse`, mettre
  `Status: resolved`, puis ajouter un renvoi (résumé + lien) aux Décisions
  acquises de `map.md`
