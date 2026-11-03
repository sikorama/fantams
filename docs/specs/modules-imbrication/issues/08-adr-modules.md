Status: resolved
Blocked by: 05, 07

# 08: ADR « les modules » (aucune n'existe)

Contenu : imbrication, fermeture par ENDMODULE, `.` seul séparateur, erreur sur
nom pointé ambigu, règle du header (`extern` refusé si défini dans le même objet).
Format : `docs/adr/00NN-...md` selon les ADR existantes (titre affirmatif, `status:`).
Les ADR 0016 (blocs) et 0027 (couture objets) sont à citer pour les règles voisines.
Pas de mention d'un autre assembleur (voir ticket 04).

## Réponse

`docs/adr/0037-les-modules-s-imbriquent-et-se-ferment.md`. Contenu : imbrication et
fermeture (décision du ticket 05), `.` seul séparateur, collision de nom pointé
= erreur à la seconde définition avec les deux côtés (ticket 07), macros restent
globales (Q2), règle du header (ADR 0027 cité), conséquences (rupture des sources
commutateur, indentation par le beautify). ADR 0016 et 0027 cités ; aucune mention
d'un autre assembleur.

À relire : la phrase « un module et une structure qui partagent un nom restent
permis tant qu'aucun membre ne porte le même nom » acte l'écart du ticket 07.
