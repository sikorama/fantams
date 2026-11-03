Status: resolved
Blocked by: —

# 09: ADR « une expansion de macro est un espace de noms »

Décisions à acter :
- Une expansion (macro, `repeat`, `while`) est un espace de noms anonyme,
  nommé par la macro et son numéro d'appel : `enum.poke__1.retry`, `enum.poke__2.retry`.
- `.` reste le seul séparateur (ticket 07 s'applique).
- Le motif `__N` est réservé : un utilisateur ne peut pas l'écrire dans un nom.
- Q1 (tranchée) : les `K EQU` plain dans une macro sont scopés par expansion,
  comme les labels. Cela remplace la règle actuelle (`pp.cpp:986`, réutilisation
  = collision volontaire) : à écrire explicitement. Partager une constante
  passe par `@@export` ou par une définition hors de la macro.
- `@@export` : reste-t-il ? (point encore ouvert : à trancher avec le développeur avant d'écrire l'ADR)
  Sens : Quel sens quand l'expansion est déjà un espace de noms ?
- Le fichier de symboles (ADR 0019) afficherait le nom par expansion.

## Décisions (tranchées le 2026-10-12)

Voir `spec.md`, « Décisions sur l'expansion » (Q6 à Q10) : labels écrits en dur
locaux à l'expansion ; labels construits par `{…}` globaux ; `@` = label global
à usage unique, sans le `@` dans le nom, erreur hors expansion ; `@@export`
supprimé ; préfixe `<macro>__<n>.` avec `__<chiffres>` réservé ; masquage comme
les modules. L'ADR consigne ces règles, l'inversion du sens de `@`, et le
remplacement de la règle `pp.cpp:986`.

## Réponse

`docs/adr/0038-une-expansion-est-un-espace-de-noms.md`. Consigne Q1 et Q6 à Q10 : labels
écrits en dur locaux (`<macro>__<n>.`), labels construits par `{…}` globaux,
`@` = global à usage unique sans `@` dans le nom (labels, constantes, instances de
structure), erreur hors expansion, `@@export` supprimé, masquage comme les modules,
`__<chiffres>` réservé, remplacement de la règle `pp.cpp:986`. Ticket 10 débloqué.
