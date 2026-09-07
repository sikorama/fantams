---
status: accepted
---

# À l'étage A, une section nomme et classe ; elle ne reloge pas

`SECTION` déclare une unité logique d'assemblage, lui donne un type, un plafond de
taille, et la porte dans la table des symboles. Elle ne change **rien** au
rangement des octets : les adresses restent celles qu'`org` décide, `emit()`
continue de ranger par banque, et le modèle mémoire de l'ADR 0006 est intact. La
migration `Space` → `Section` du §10 de la spécification attend l'étage B.

## Contexte

Le §10 décrit une généralisation qui semble aller de soi : un `Space` est déjà
« un bloc d'octets avec sa propre coverage, alloué à la première écriture, indexé
par un entier dont on ne préjuge pas le sens » ; il suffirait que la clé cesse
d'être un numéro de banque pour devenir une identité de section, et `emit()`
changerait de trois lignes.

Trois lignes, mais elles ne sont pas seules. Une section qui **range** est une
section dont les adresses sont relatives à `0x0000` : la valeur d'un symbole
devient `(section, offset)`, l'évaluateur d'expressions doit savoir qu'une
expression est **affine** en bases de sections, et un saut relatif inter-sections
devient une relocalisation. C'est une reprise d'`expr.cpp` et de la table des
symboles — le seul morceau du découpage qui ne s'écrit pas en petits pas.

Or les trois gains que l'étage A vise ne demandent aucune de ces choses :

- le **plafond de taille** est un compteur d'octets ;
- la **détection d'écriture en `"ro"`** est une lecture du type de la section qui
  porte un symbole ;
- la **section dans `--sym`** est une colonne de plus dans une table qui existe.

## Décision

Une section, à l'étage A, **nomme et classe**. Elle est déclarative : elle dit ce
qu'un bloc de source *est*, pas où il *va*.

Ce qui suit de cette décision, et qu'il faut lire ensemble :

**La taille d'une section est la somme des octets émis**, cumulée sur toutes ses
réouvertures — et non l'étendue `max − min` de ses adresses. Ce qui compte est la
place qu'une section demande, celle que le linker posera d'un bloc ; l'intervalle
qu'elle couvre, avec `org` absolu à l'intérieur, serait un nombre sans
signification. Une place **réservée** compte comme une place écrite : c'est la
seule information qu'une section `"uninit"` donne au linker.

**`align` et `boundary` n'y comptent pas.** Ils avancent l'adresse sans émettre ni
réserver, et à cet étage le remplissage n'existe pas — c'est `org` qui décide des
adresses, donc personne n'a à remplir quoi que ce soit. **À recompter à l'étage
C** : quand c'est le linker qui aligne, le remplissage devient de la place réelle,
et la taille d'une section devra le comprendre. C'est le seul endroit où cette
décision change de réponse, et c'est pourquoi elle est écrite ici plutôt que
laissée à deviner dans `emit()`.

**Le type et le plafond sont figés à la première déclaration.** Une section se
rouvre — c'est ainsi qu'on alterne code et données — et la réouverture peut
omettre l'un et l'autre ; elle ne peut ni les changer, ni en introduire un que la
première déclaration ne portait pas. La raison est la même dans les deux cas :
rouvrir en `"rw"` ce qui a été déclaré `"ro"` désarmerait le contrôle d'écriture,
et relever un plafond depuis un fichier inclus le désarmerait de même. Un contrôle
qu'on désarme en silence est pire qu'un contrôle absent, parce que son auteur
croit l'avoir.

**Une section `"uninit"` n'émet pas d'octet.** `ds` y réserve — l'adresse avance,
la coverage ne bouge pas, rien n'entre dans l'image — et tout le reste y est
refusé. C'est ce qui laisse intacte la fusion avec une base (ADR 0012), qui ne
recopie que ce qui est couvert.

## Ce que l'étage A ne fait pas, et pourquoi ce n'est pas un travail à moitié fait

`PUBLIC` / `EXTERN`, le fichier objet, la table des accès à adresse littérale : ils
demandent tous la relocalisation, donc l'étage B. Ce sont des **manques annoncés**,
pas des restes. Les étages A et B sont indépendants, et A se tient seul : le
plafond, le refus d'écriture en ROM et le contrôle de taille d'une sous-zone sont
utiles sans linker, se testent sans linker, et ne se paient pas dans `expr.cpp`.

L'écart avec la lettre du §10 est donc assumé, et il a une date de fin : le jour où
un fichier objet doit sortir de l'assembleur, la migration devient nécessaire — et
pas un jour avant.

## Conséquences

- `emit()` reste le seul point de passage des octets, et le devient davantage : le
  compteur de taille et le refus d'émission en `"uninit"` y sont, ce qui dispense
  de reprendre `db`, `dw`, les chaînes et l'encodeur un par un.
- Le contrôle de taille sort **à l'assemblage**, sans attendre le linkage. Trois
  fautes qui demandaient un linker sont attrapées sans lui : écriture en ROM,
  dépassement d'une section, dépassement d'une sous-zone.
- La table des symboles porte la section de chaque symbole. C'est l'embryon du
  fichier objet, et le premier consommateur externe peut déjà la lire.
- Le jour de la migration, rien de ce qui est écrit ici n'est perdu : les types,
  les plafonds et les noms sont exactement ce qu'un fichier objet doit porter. Ce
  qui change est le rangement des octets, et lui seul.
