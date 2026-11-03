Status: resolved
Blocked by: —

# 03: Corriger la doc sur @@export, documenter `extern` qualifié et le pattern header

1. `docs/syntax.md` (≈ l.898-906) et `CONTEXT.md:135` disent que `@@export`
   fait partager un nom à toutes les expansions. C'est faux : la deuxième
   expansion donne `duplicate symbol`. Décrire le comportement réel : `@@export`
   sort le label du renommage, il ne sert qu'à une macro développée une fois.
2. `docs/syntax.md`, section « Scope between objects » : dans un module, un nom
   exporté s'importe qualifié (`extern lib.init`, pas `extern init`). Le texte
   actuel ne le dit pas.
3. Ajouter le pattern header, sans changer la règle : un header commun
   (constantes `EQU` + gardes `ifndef`) est inclus par la bibliothèque et par
   ses utilisateurs. Les `extern` vont dans un fichier séparé, inclus seulement
   par les utilisateurs. Un header contenant `extern` et `public` du même nom
   est refusé (règle conservée, à expliquer).

**Critère** : chaque affirmation de la doc est vérifiée par un cas réel
(voir les tests manuels de cette session : header gardé linké, `extern init`
refusé dans un module).

## Réponse

`docs/syntax.md` : paragraphe `@@export` corrigé (nom gardé tel quel, pas
partagé, une seule expansion) ; section « Scope between objects » complétée
(`extern lib.init` qualifié, `public init` dans le module, pattern header).
`CONTEXT.md` : définition de « Scope auto-local » corrigée. Le futur sens de
`@@export` n'est pas promis (voir ticket 09).

Vérifié avec `fantams -o x.fo` puis édition de liens : `extern init` (module ou
non) → `unresolved EXTERN symbol 'init'` ; `extern lib.init` se résout ;
header gardé inclus des deux côtés sans erreur de symbole ; `extern` + définition
du même nom dans un objet → refusé, au header comme au fichier. Non vérifié : un
binaire final (mes objets de test se recouvraient, il faudrait un script de
sections).
