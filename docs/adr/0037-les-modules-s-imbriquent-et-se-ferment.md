---
status: accepted
---

# Un module est un bloc : il s'ouvre, s'imbrique, se ferme, et `.` est le seul séparateur

`MODULE nom` **ouvre** un module, `ENDMODULE` (ou `end`) le ferme. Les modules
s'imbriquent : `MODULE chose` dans `MODULE toto` donne `toto.chose.ADDR`, et chaque
fermeture ferme le niveau le plus interne. La forme commutateur — un `MODULE x`
qui remplace le module actif sans fermeture — et `MODULE OFF` n'existent plus.

`.` est le **seul** séparateur de noms. Un nom pointé qui peut venir de plusieurs
chemins n'a pas de règle de syntaxe propre : il est défini deux fois, et la
seconde définition est une erreur qui nomme les deux côtés.

## Contexte

`MODULE` est né comme un commutateur : `MODULE x` remplaçait le module actif,
`MODULE OFF` ou `ENDMODULE` le désactivait. Aucun ADR ne le décrivait, et le
préprocesseur le tenait **hors** de la table des blocs (ADR 0016) — « deux
`MODULE` pour un `ENDMODULE` » était la forme normale, et compter `MODULE` parmi
les ouvreurs aurait déséquilibré la pile.

Deux faits ont rendu cette forme intenable :

- **Elle ne composait pas.** Un utilisateur qui construit des bibliothèques
  réutilisables veut un module dans un module, ou des structures préfixées à
  l'intérieur d'un module. Une seconde construction (`NS` / `ENDNS`) aurait
  doublé le mécanisme pour un besoin que le premier couvre dès qu'il s'empile.
- **Elle cachait des défauts.** Le renommage est textuel et se faisait par
  bloc : une constante définie dans un premier `MODULE a` était invisible dans
  un second `MODULE a` (`unknown symbol 'CONST'`, alors que `a.CONST` existait).
  Et un `@label` de macro dans un module perdait son auto-unicité à la seconde
  expansion, le préfixe de module cachant le `@` à la règle qui le cherchait.

## Décision

**Un module est un bloc comme les autres.** Il entre dans la table des blocs
avec sa fermeture `ENDMODULE`, `end` ferme le plus interne, et la correspondance
est vérifiée (ADR 0016). Cela donne une syntaxe régulière : tout ce qui a un
corps s'ouvre et se ferme. Les erreurs sont nommées — `MODULE` sans fermeture,
`ENDMODULE` orphelin, `MODULE` sans nom, `MODULE OFF` (qui dit comment on ferme
un module à la place).

**Le préfixe se compose** : `préfixe externe + nom + "."`. Un module interne garde
ses propres noms : ce qu'il définit lui-même masque le même nom du module
englobant, et un nom que seul l'englobant définit reste visible sans préfixe à
l'intérieur. La ligne `MODULE x` n'est jamais renommée par l'extérieur.

**Les noms d'un module sont ceux de tous ses blocs.** Rouvrir un module (un second
`MODULE toto`) voit les constantes et labels des blocs frères, avant comme
après. La table est indexée par le chemin complet du module. Pour un module
imbriqué rouvert, seuls les blocs frères **déjà rencontrés** sont vus : la table
s'alimente au passage.

**Les champs de structure ne sont jamais préfixés par le module** : ils sont
qualifiés par leur structure (`toto.S_COLORS.HUD`), et on les coupe au dernier
point. Hors du module, un nom s'écrit qualifié, sans recherche de repli entre
modules.

**`.` est le seul séparateur.** `gfx.plot.x` est aussi bien le local `.x` du label
`gfx.plot`, le label `x` d'un module `gfx.plot`, que le champ `x` d'une structure
`gfx.plot`. On ne distingue pas ces origines par la syntaxe : le même chemin
complet défini deux fois est une collision, refusée à la **seconde définition**,
avec les deux côtés — la forme (« local label '.x' of 'gfx.plot' », « a label »,
« a constant ») et le `fichier:ligne` de la première. Un module et une structure
(ou un label) qui partagent un nom restent permis tant qu'aucun membre ne porte
le même nom.

**Les macros** étaient globales dans cette décision ; l'ADR 0039 les range dans le
module où elles sont définies (`m.a()`).

## La règle du header

Une bibliothèque et ses utilisateurs partagent un **header commun** — constantes
`EQU` gardées par `ifndef` — inclus des deux côtés. Les `extern` vont dans un
fichier réservé aux utilisateurs. Un objet qui à la fois déclare `extern` et
définit le même nom est **refusé** (ADR 0027) : le header qui contiendrait les
deux serait lu aussi par la bibliothèque, qui définirait et importerait le même
nom. La règle est conservée telle quelle ; ce qui change est de la rendre
lisible, pas de la relâcher.

Dans un module, un nom exporté est qualifié : `public init` s'écrit dans le
module, `extern lib.init` s'écrit chez l'utilisateur. `extern init` nomme un
symbole qu'aucun objet n'exporte et échoue à l'édition de liens.

## Conséquences

- **Rupture assumée.** Une source qui enchaînait `MODULE a` puis `MODULE b` sans
  `ENDMODULE`, ou qui fermait par `MODULE OFF`, est refusée avec un message qui dit
  quoi écrire. Le correctif est mécanique, et c'est au développeur de la source de
  l'appliquer.
- **La mise en forme indente le corps d'un module**, comme celui de tout bloc
  (ADR 0013, règle 4) : `MODULE` n'est plus une exception qui n'ouvre pas de
  bloc.
- **Une collision de nom pointé est une erreur ordinaire de l'assembleur**, pas une
  règle du préprocesseur : l'assembleur voit les noms après préprocesseur, donc il
  ne dit ni « module » ni « champ de structure » — seulement la forme du nom (label,
  local, constante). C'est suffisant pour que les deux côtés soient lisibles ; dire
  la provenance module/structure demanderait au préprocesseur de la transmettre.
- **Hors périmètre** : le scope par expansion de macro (un espace de noms par
  appel) fait l'objet d'une décision séparée.
