---
status: accepted
---

# La configuration est de première classe ; `SHADOWS` n'existe pas

Le modèle mémoire du linker décrit des **configurations** — des états de carte
nommés, énumérés par le profil de cible — et non une sélection par fenêtre. Les
attributs qui se déduisent de la comparaison de deux configurations ne se
déclarent pas : ils se calculent. `SHADOWS` et `ALWAYS` disparaissent donc du
vocabulaire.

## Contexte

La première formulation du modèle était `SELECT <fenêtre>, <banque>` : chaque
fenêtre de l'espace adressable choisit indépendamment la banque qui s'y montre.
C'est la lecture qu'on obtient en ne regardant qu'un mécanisme de commutation à
la fois, et elle est fausse sur les trois machines étudiées :

- CPC, configuration `%011` : le bloc 3 de la RAM de base passe en `&4000`
  **et** le bloc 3 de la page étendue en `&C000`, d'un seul geste ;
- CPC, configuration `%010` : les quatre fenêtres basculent ensemble ;
- ZX +2A/+3, mode spécial : exactement quatre combinaisons figées —
  `(0,1,2,3)`, `(4,5,6,7)`, `(4,5,6,3)`, `(4,7,6,3)`.

Aucune n'est décomposable en choix par fenêtre. À l'inverse, le PPI du MSX
*est* authentiquement indépendant : deux bits par page, produit cartésien
complet.

Le modèle par fenêtre aurait donc traité comme cas général ce qui est le cas
particulier, et exigé un cas particulier pour la règle. Les valeurs venaient de
`docs/recherche/cpc-gate-array-rmr.md` et
`docs/recherche/zx128-msx-pagination.md`, vérifiées sur sources primaires.

## Décision

Un profil déclare un ou plusieurs **axes** de configuration ; l'état de la
machine est le produit des axes, et l'indépendance est décrite
paramétriquement plutôt qu'écrite à la main. Un axe peut en recouvrir un autre
(`OVER`), parce qu'une ROM cache la RAM **en lecture** seulement.

Conséquence directe, et c'est la moitié de l'intérêt : ce que masque une
commutation cesse d'être une déclaration. Deux configurations d'un même axe qui
n'accordent pas la même banque à une fenêtre disent, par leur seule existence,
que passer de l'une à l'autre fait disparaître ce qui était là. Une fenêtre à
laquelle toutes les configurations donnent la même banque est fixe, sans qu'on
l'écrive.

## Conséquences

- **Une source d'erreur de saisie supprimée.** `SHADOWS` était l'attribut le
  plus facile à écrire faux dans un profil, et le plus coûteux : son oubli
  laisse produire un programme qui se sabote à la première commutation.
- **Le contrôle de co-visibilité devient décidable sans énumérer.** Le produit
  des axes est immense et le linker ne connaît aucune séquence d'exécution ;
  « deux banques sont co-visibles si aucun axe ne les met dans la même
  fenêtre » se décide axe par axe, à coût constant.
- **Le profil grossit.** Énumérer huit configurations est plus verbeux que
  déclarer quatre fenêtres, et c'est le prix accepté : ce qui est énuméré est ce
  que le matériel sait réellement faire.

## Alternatives écartées

- **Rester par fenêtre, traiter CPC et +3 en cas particuliers.** C'est la forme
  qui rend le linker plein de cas particuliers CPC, exactement ce que le
  découpage assembleur / linker devait éviter.
- **Admettre les deux formes.** Deux façons d'écrire un profil, dont l'une ne
  peut pas tout dire : la moins expressive serait employée par défaut, et la
  frontière pourrirait faute d'être exercée.
