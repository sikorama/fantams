---
status: accepted
---

# Une expansion est un espace de noms : ce que la macro nomme est local, ce que l'appelant nomme est à lui

Une expansion — d'une macro, d'un `repeat`, d'un `while`, d'un `for` — est un
espace de noms. Un label **écrit en dur** dans son corps est **local** à chaque
expansion. Un label dont le nom **contient une substitution `{…}`** est **global** :
c'est l'appelant qui l'a nommé. Le marqueur **`@`** sort du lot un label écrit en
dur : il reste global, tel quel, et la macro ne peut alors être développée qu'une
fois. `@@export` disparaît.

## Contexte

Le renommage par expansion était **opt-in** : seul un label préfixé par `@` recevait
un suffixe `__<n>` unique par expansion. Un label ordinaire n'était pas renommé :
réutilisé entre deux expansions, il restait une vraie collision (`duplicate symbol`).
`@@export name` sortait un label `@` de ce renommage pour qu'il garde son nom.

Cette règle avait trois défauts :

- **Le défaut était le piège.** Une macro avec une boucle (`loop: djnz loop`)
  casse à la seconde expansion, et l'auteur doit penser à écrire `@loop`. Écrire
  correctement coûtait un signe ; écrire naïvement coûtait une erreur.
- **`@@export` ne faisait pas ce que la doc disait.** Il ne fait pas partager un nom
  entre expansions : la seconde donne `duplicate symbol`. Il n'a de sens que pour
  une macro développée une fois, et il fallait une directive à part pour le dire.
- **Elle composait mal avec les modules.** Le préfixe de module cachait le `@` à
  la règle qui le cherchait : un `@r` de macro dans un module n'avait plus son
  suffixe à la seconde expansion. Le correctif rendait `e.@r` reconnaissable, ce qui
  prolongeait un mécanisme à deux couches là où un seul espace de noms suffisait.

## Décision

**Les labels écrits en dur sont locaux à l'expansion.** Le préfixe d'expansion est
`<macro>__<n>.` : une macro `poke` appelée deux fois définit `poke__1.retry` et
`poke__2.retry`. Un `repeat`, un `while` ou un `for` prend son mot-clé
(`repeat__3.x`). Les expansions imbriquées se composent comme les modules
(`m__1.repeat__2.x`), et dans un module le préfixe de module précède celui de
l'expansion. Le numéro est un compteur global d'appels. Les constantes (`K EQU`) suivent
la même règle : c'était l'exception volontaire de `pp.cpp` (« une constante
réutilisée entre expansions est une collision »), et elle est **remplacée** : une
expansion qui est un espace de noms mais dont les constantes fuiraient serait
incohérente.

**Ce que l'appelant nomme est à lui.** Un label dont le nom contient une
substitution — `{name}:`, `mon_label_{name}:`, `{name}_label:`, `{name}.fin:` — garde
le nom construit, quelle que soit la place des accolades. C'est la forme stable pour
exposer un point d'entrée. Deux appels qui construisent le même nom sont une
erreur (`duplicate symbol`), voulue : c'est une vraie collision de noms choisis par
l'appelant.

**`@` marque un label global à usage unique.** `@entry:` définit `entry` — le `@`
n'est pas dans le nom, qu'on écrit `entry` depuis l'extérieur — et la macro ne peut
être développée qu'une fois : un second appel donne `duplicate symbol: 'entry'`, dont
le message dit que la macro porte un label `@`. Même marqueur pour une constante
(`@K EQU 5`) et pour une instance de structure (`STRUCT type @x`). `@x` en dehors
d'une expansion — au niveau supérieur ou dans un module — est une erreur nommée : il
n'y a pas d'expansion à marquer, et tout label y est déjà global.

**`@@export` disparaît**, avec une erreur nommée qui renvoie à `@` sur le label.

**Un nom local masque le même nom global ; les autres restent visibles.** Dans une
macro qui définit `loop:`, `loop` désigne son propre label même s'il existe un
`loop` global. Un nom que l'expansion ne définit pas (une table globale, par
exemple) reste visible tel quel. C'est la règle des modules imbriqués (ADR 0037).

**Le motif `__<chiffres>` est réservé** : un nom écrit par l'utilisateur ne peut pas
le porter, sans quoi `poke__1.retry` pourrait être aussi un nom écrit à la main. Le
fichier de symboles (ADR 0019) et `--sym` montrent les noms par expansion tels
quels.

## Conséquences

- **Le sens de `@` s'inverse.** Il voulait dire « unique par expansion », il dit
  « global, une seule expansion ». C'est sûr : une macro développée plusieurs fois
  avec l'ancien `@` casse aussitôt sur un `duplicate symbol` et le développeur
  retire le `@` ; une macro développée une fois continue de marcher. Les sources et
  les exemples qui écrivent `@` (`examples/demo.asm`) sont à corriger par le
  développeur.
- **Le bug du `@` de macro dans un module disparaît par construction** : il n'y a
  plus de suffixe `@` à protéger, et le correctif provisoire (la reconnaissance de
  `e.@x`) est supprimé. Un test le garde.
- **Un label écrit en dur n'est plus atteignable depuis l'extérieur** de son
  expansion, sauf à le marquer `@` ou à le construire par argument. C'est voulu :
  ce que la macro nomme elle-même est son affaire.
- **Écartée** : un `{}` vide dans le nom pour marquer un label global. Peu
  élégant, et il change de sens si un paramètre passé est vide.
- **Hors périmètre** : le scope des macros elles-mêmes par module (ADR 0037).
