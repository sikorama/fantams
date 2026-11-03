# Calculer la forme d'un corps de boucle une seule fois

Status: needs-triage
Type: task

Chaque tour rappelle `findMatching` pour chaque `if` du corps, puis recherche
les branches `else`, et copie la branche retenue dans un nouveau vecteur.
`classify` est mémoïsée, mais la mémoïsation hache le texte entier de chaque
ligne. Une « forme » calculée par corps (fermeture de chaque ouvreur, bornes
des branches, formes des sous-corps) supprimerait ce travail.

- Gain estimé : ≈ 10 % du préprocesseur.
- Complexité : moyenne à forte. Il faut passer la forme à `run()` et aux
  récursions (branches d'`if`, boucles imbriquées), et `renameLocals` change le
  texte des corps (labels `@x__n`) sans en changer la forme.
