# AGENTS.md

Conventions à l'usage des agents qui travaillent dans ce dépôt.

## Agent skills

### Issue tracker

Les issues et les specs vivent en markdown sous `.scratch/<chantier>/`.
Voir `docs/agents/issue-tracker.md`.

### Triage labels

Les cinq rôles canoniques, sans renommage. Voir `docs/agents/triage-labels.md`.

### Domain docs

Contexte unique : `CONTEXT.md` et `docs/adr/` à la racine.
Voir `docs/agents/domain.md`.

### Revue de code

Un diff se relit selon `docs/agents/review.md` : reproduire avant d'affirmer,
rapporter les verrous sautés.

### Boucle d'implémentation

`make test-fast` en cours de travail ; `make test` (plus de deux minutes) une
fois, avant le commit. Les deux reconstruisent `ppdump`.
