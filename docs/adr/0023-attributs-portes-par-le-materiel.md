---
status: accepted
---

# Les attributs se portent là où le matériel les porte

La contention, la lecture seule matérielle et la visibilité par le contrôleur
vidéo sont des attributs de **banque**. La lecture seule *effective* d'une
fenêtre est une propriété de **configuration**, déduite et non déclarée.
L'irréversibilité d'une commutation est un attribut de la **pagination entière**,
jamais d'un port. Trois porteurs, et non un.

## Contexte

La première version du vocabulaire mettait tous les attributs sur la fenêtre ou
sur la banque, indistinctement — un profil ZX écrivait
`WINDOW low [&4000..&7FFF] ALWAYS bank5 CONTENDED`. La vérification
documentaire (`docs/recherche/zx128-msx-pagination.md`) a montré que ce
raccourci est juste par accident et faux comme modèle :

- la **contention** frappe des **banques** RAM : 1, 3, 5 et 7 sur ZX 128 et +2
  — par une erreur d'équations du PAL, le service manual documentant
  l'intention et non le matériel livré — et 4 à 7 sur +2A/+3. La fenêtre
  `&C000`-`&FFFF` est donc lente ou non *selon ce qui y est paginé*, ce qu'un
  attribut de fenêtre ne peut pas exprimer ;
- la **lecture seule** d'une fenêtre dépend de l'état : en mode spécial des
  +2A/+3, `&0000`-`&3FFF` porte de la RAM. « La fenêtre ROM est `READONLY` » y
  est faux, alors que « cette banque est une ROM » reste vrai partout ;
- le **verrou** du ZX (bit 5 de `&7FFD`) bloque aussi le port `&1FFD` : il ne
  qualifie pas un port mais la pagination.

S'y ajoute un attribut que le modèle n'avait pas et qui interdit un placement :
sur CPC, la RAM étendue **n'est jamais lue par le CRTC**, donc aucune section
écran ne peut y vivre (`docs/recherche/cpc-gate-array-rmr.md`).

## Décision

| attribut | porteur |
|---|---|
| `CONTENDED` | banque |
| `ro` / `rw` matériel | banque |
| `VIDEO` | banque |
| lecture seule *effective* d'une fenêtre | configuration (déduite) |
| `LOCKS` | pagination entière |

## Conséquences

- Un refus de placement nouveau, et que seul le linker peut prononcer : une
  section écran dans une banque sans `VIDEO`.
- Le conseil « pas de code au timing critique en `&4000` » sur ZX cesse d'être
  une règle d'adresse pour devenir une conséquence : la banque 5, toujours
  présente dans cette fenêtre, est contended — environ 25 % de débit en moins.
- Un profil ne peut plus mentir par raccourci : dire `CONTENDED` d'une fenêtre
  serait devenu un mot sans porteur.
