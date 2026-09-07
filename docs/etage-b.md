# Étage B — suivi

> **Ce fichier tient lieu de ticket.** Une ligne d'état par étape, mise à jour au
> fur et à mesure ; il n'y a rien d'autre à synchroniser. Le *quoi* est dans
> [spec-etage-b.md](spec-etage-b.md), le *où* dans
> [coutures-de-la-chaine.md](coutures-de-la-chaine.md) ; ici, l'ordre, les arêtes
> et l'état.

**Ce qu'est l'étage B**, d'après le §10 : fichier objet et expressions
relocalisables — compilation séparée, `PUBLIC` / `EXTERN`, tailles résolues au
linkage. C'est le gros morceau, et le seul que le §10 déclare indivisible.

**Ce qu'il n'est pas.** Le linker de l'étage B **place absolument**, tel qu'`org`
le dit : il ne calcule ni fenêtre, ni banque, ni configuration, et ne vérifie
aucune continuité. C1 remplacera son intérieur sans toucher à son interface.
C'est ce qui permet de livrer B sans promettre C.

**L'appel de conception de l'étage** est que l'indivisibilité annoncée porte sur
**une seule étape** : la valeur affine de l'évaluateur d'expressions (étape B1)
se teste seule, avec un résolveur rendant des sections factices, sans un octet
d'assembleur. Le reste se découpe.

**L'invariant qui fixe l'ordre** : le dépôt compile et les sept suites sont
vertes à chaque étape, comme pendant tout l'étage A.

---

## État

| # | Étape | Bloqué par | État |
|---|-------|-----------|------|
| B0 | Une seule table de section *(préfacteur)* | — | **faite** |
| B1 | La valeur affine dans l'évaluateur d'expressions | — | **faite** |
| B2 | Le fragment | B0 | à faire |
| B3 | La couture du linker | B2 | à faire |
| B4 | La table des symboles produite par le linker | B3 | à faire |
| B5 | La relocalisation de bout en bout | B1, B3 | à faire |
| B6 | `PUBLIC` et `EXTERN` | B5 | à faire |
| B7 | Le fichier objet, aller-retour | B6 | à faire |
| B8 | Le multi-objet et l'exemple d'acceptation | B7 | à faire |
| B9 | Les accès à adresse littérale dans l'objet | B3 | à faire |
| B10 | L'ADR de clôture | B1, B2, B7, B8, B9 | à faire |

Deux étapes ne sont pas dans la chaîne : **B1 est parallèle à B0**, et **B9 ne
dépend que de B3**.

Fin de l'étage B : B0 à B10 faites, les sept suites vertes plus celle du linkage,
`docs/syntax.md` à jour, et l'exemple d'acceptation de B8 qui produit le binaire
identique octet pour octet.

---

## B0 — une seule table de section *(préfacteur)*

**Ce qu'il livre.** Rien, du point de vue de l'utilisateur, et c'est l'énoncé :
aucun message, aucun octet, aucune adresse ne change. Les quatre tables indexées
par nom de section que l'étage A a laissées en parallèle du modèle mémoire —
type, plafond déclaré, site de la déclaration qui porte le plafond, taille
cumulée — deviennent une table unique d'enregistrements.

**Pourquoi d'abord.** C'est ce qui rend B2 petit : la section est l'objet dans
lequel le fragment va se ranger, et le fabriquer avant d'y toucher évite de mêler
deux changements dans la même relecture.

- [x] Les quatre tables sont remplacées par une seule, indexée par nom de section
- [x] Les sept suites vertes, **sans une ligne de test modifiée**
- [x] Aucun message de diagnostic n'a changé d'un caractère

## B1 — la valeur affine dans l'évaluateur d'expressions

**Ce qu'il livre.** Une expression sait dire qu'elle vaut *une base de section
plus un décalage* au lieu d'un nombre seul, et refuse ce qui n'a pas de sens dans
cet état. `high(x)` et `low(x)` apparaissent.

Le préprocesseur, qui tourne avant qu'aucune adresse existe, n'a jamais de valeur
relocalisable à rendre : il n'est touché qu'à l'endroit où il fabrique son
résolveur.

- [x] `fin - debut` reste un nombre quand les deux labels partagent leur section
- [x] `label` seul est relocalisable
- [x] `label * 2` et `label + label` sont refusés, avec un message qui dit pourquoi
- [x] `label_s1 - label_s2`, sections différentes, est refusé
- [x] `high(x)` et `low(x)` rendent les deux octets d'une valeur relocalisable
- [x] `label >> 8` et `label & 255` sur une valeur relocalisable sont refusés **en nommant `high()` / `low()`**
- [x] Les mêmes formes restent légales sur une valeur absolue
- [x] Une valeur relocalisable ne peut pas porter de partie réelle non entière
- [x] Tout cela est testé à la couture de l'évaluateur, avec un résolveur de test rendant des sections factices, sans assembler une ligne
- [x] Les sept suites vertes : l'assembleur rend encore une valeur absolue partout

Deux choix pris en cours de route, à relire en B10 :

- **`hi()` / `lo()` sont des graphies de `high()` / `low()`**, valeur
  relocalisable comprise. Deux fonctions dont une seule accepterait une adresse
  serait une asymétrie qu'aucune règle ne fait deviner ; le refus de `>> 8`
  nomme quand même la graphie canonique.
- **L'unaire moins garde l'affinité** et rend un coefficient `-1`. C'est ce qui
  permet à `fin + -debut` de s'annuler ; un `-1` final n'est pas émettable, mais
  ce n'est pas à l'évaluateur d'en juger — ce sera à l'émetteur, en B5.

## B2 — le fragment

**Ce qu'il livre.** Un `org` à l'intérieur d'une section y ouvre un fragment ; une
section sans `org` est un fragment unique. C'est ce qui donne à « relocalisable »
une définition sans nouvelle syntaxe, et cela ne change rien pour l'auteur d'une
source d'aujourd'hui — qui porte un `org` dans chaque section.

- [ ] Les octets vont dans (fragment courant, offset courant), et non plus à une banque dérivée de l'adresse
- [ ] Un fragment porte sa propre coverage, allouée à la première écriture
- [ ] Une section porte une liste de fragments, chacun connaissant son adresse absolue si un `org` la lui a donnée
- [ ] Le plafond de section, `"uninit"`, `BOUNDARY` et `ASSERT_SIZE` se comportent à l'identique
- [ ] Les sept suites vertes, **sans une ligne de test modifiée** — c'est la preuve de la migration

## B3 — la couture du linker

**Ce qu'il livre.** L'assembleur rend un **objet**, le linker rend une **image**,
et le CLI consomme l'image. Le binaire et le snapshot produits sont identiques à
ceux d'avant : c'est une migration, pas une fonctionnalité.

Le travail de linker aujourd'hui dispersé — la dérivation banque↔adresse, la
limite des banques qu'un dump plat sait porter, le choix 64 K / 128 K, la
détection de recouvrement — migre derrière la couture. Le CLI n'en dérive plus
aucun.

Le backend de snapshot n'est **pas** touché : sa signature plate est un problème
réel, mais c'est celui du builder, et le mêler ici ferait de l'étage indivisible
un étage à deux sujets.

- [ ] L'assembleur ne rend plus ni image plate, ni coverage parallèle, ni banques écrites, ni binaire, ni adresse de chargement, ni adresse d'exécution
- [ ] L'entrée du programme est un **nom** de symbole, résolu par le linker
- [ ] La coverage voyage attachée à ses octets, du fragment jusqu'au bloc placé, jamais en paramètre optionnel parallèle
- [ ] Une nouvelle suite teste le linkage à partir d'objets fabriqués à la main
- [ ] Cette suite est inscrite dans **les deux** listes de tests du dépôt
- [ ] Les 730 assertions de la suite d'assemblage survivent par un helper de harnais qui assemble puis linke ; ce helper n'est pas exposé comme API
- [ ] La suite de snapshot verte : la fusion avec une base ne recopie toujours que ce qui est réellement écrit
- [ ] Le binaire et le snapshot produits par le CLI sont inchangés

## B4 — la table des symboles produite par le linker

**Ce qu'il livre.** Dans une section relocalisable, un label n'a pas d'adresse :
la table exportable ne peut plus sortir de l'assembleur. Elle change de maillon
**sans changer de format** — et son consommateur, désassembleur ou émulateur, ne
voit pas la différence. C'est même la raison de le faire ainsi.

- [ ] La table est produite par le linker, avec des adresses définitives
- [ ] Le format ne change pas d'une colonne ni d'un en-tête
- [ ] L'amendement à l'ADR 0019 est écrit **dans cette étape**, pas à la fin de l'étage

## B5 — la relocalisation de bout en bout

**Ce qu'il livre.** Ce pour quoi l'étage existe : une section sans `org` est
relocalisable, et c'est le linker qui la place. Les quatre types de
relocalisation sont émis et résolus.

**Correction du §10**, à écrire ici : l'encodeur *est* concerné par la migration,
contrairement à ce que le §10 affirme. Il calcule lui-même le déplacement d'un
saut relatif et ne peut plus le faire sur une cible relocalisable ; l'évaluation
qui lui est offerte rend donc une valeur, il en teste le coefficient et demande
une relocalisation. Le fait — *cette cible n'est pas connue* — appartient à
l'endroit qui l'encode.

- [ ] Une section sans `org` est relocalisable et placée par le linker
- [ ] `Abs16`, `Rel8`, `High8` et `Low8` sont émises par l'assembleur et résolues par le linker
- [ ] Un saut relatif hors de portée est refusé par l'encodeur en intra-section, où il connaît la distance
- [ ] Le même, inter-sections, est refusé par le linker, où lui seul la connaît
- [ ] `$` se comporte comme un label de la section courante
- [ ] La portée hors de [-128, 127] n'est jamais silencieuse : aucun octet de garde n'est émis sans relocalisation
- [ ] La correction du §10 est écrite dans cette étape

## B6 — `PUBLIC` et `EXTERN`

**Ce qu'il livre.** Les deux directives du §4.4, et la portée par défaut qui les
rend utiles. Encore un seul objet à cette étape : le mécanisme existe, rien ne
l'exerce — c'est B8 qui l'exercera.

Un nom ni défini ni déclaré `EXTERN` reste une **erreur d'assemblage**. Un
`EXTERN` implicite transformerait une faute de frappe en relocalisation non
résolue, signalée deux maillons plus loin, alors qu'aujourd'hui l'assembleur la
dit à la bonne ligne.

- [ ] Un symbole est local à son objet par défaut
- [ ] `PUBLIC` l'exporte, `EXTERN` le déclare défini ailleurs
- [ ] Un nom inconnu et non déclaré `EXTERN` est une erreur d'assemblage, à sa ligne
- [ ] La mise en forme connaît `public`, `extern`, `high` et `low` — un mot réservé l'est à toutes les phases (ADR 0015)
- [ ] Un de ces mots seul en colonne 1 ne reçoit pas de deux-points (la faute d'A4, qui détruisait le source)
- [ ] L'idempotence et l'invariant d'octets de la mise en forme tiennent sur une source qui les porte

## B7 — le fichier objet, aller-retour

**Ce qu'il livre.** Un objet qui s'écrit et se relit. Extension `.fo`, format
texte à blocs nommés — sections et fragments, symboles, relocalisations, accès
littéraux, octets en hexadécimal — lisible dans un terminal, comparable par
chaîne dans un test.

Pas de format compact : un objet faux se lit à l'œil, ce qui vaut plus que tout à
l'étage qui introduit la relocalisation, et la couture *format* reste
hypothétique jusqu'à l'étage D.

- [ ] Assembler seul écrit un `.fo`
- [ ] fantams relit un `.fo`
- [ ] L'aller-retour est stable : écrire, relire, réécrire, et les deux textes sont identiques
- [ ] Le fichier est lisible et compréhensible sans outil, à l'écran
- [ ] Un `.fo` malformé est refusé avec un diagnostic qui nomme la ligne fautive
- [ ] La table des symboles exportable en reste un sous-ensemble

## B8 — le multi-objet et l'exemple d'acceptation

**Ce qu'il livre.** La compilation séparée, réellement livrée : N objets linkés en
un binaire, et les diagnostics qui vont avec. Sans cette étape, `EXTERN` serait
un mécanisme qu'aucun test ne peut exercer et `PUBLIC` ne distinguerait rien —
l'état que l'étage A a refusé trois fois.

**Le critère de fin d'étage** est ici.

- [ ] N objets sont linkés en un binaire
- [ ] Une relocalisation qu'aucun objet ne résout est refusée, en nommant le symbole et l'objet qui le demande
- [ ] Deux définitions du même symbole exporté sont refusées, en nommant les deux provenances
- [ ] Deux fragments qui se recouvrent sont refusés
- [ ] Un exemple d'acceptation dans `examples/` : deux sources portant une section relocalisable, un `PUBLIC` / `EXTERN`, un saut relatif inter-sections et un `high()`
- [ ] Assemblées **séparément** puis linkées, elles produisent un binaire **identique octet pour octet** à celui de la version monolithique équivalente

## B9 — les accès à adresse littérale dans l'objet

**Ce qu'il livre.** Le quatrième bloc du §4.6, en **écritures mémoire
seulement** : le sens que le contrôle d'écriture en `"ro"` de l'étage A produit
déjà et teste déjà. Aucun consommateur avant C2 — l'assembleur consigne, il
n'interprète pas.

L'intérêt de le faire ici est de valider le format objet sur un bloc qui n'est ni
des octets ni des symboles. Les lectures et les ports demandent un parcours
d'encodeur que rien ne consomme avant C2 ; les ajouter tard est indolore.

- [ ] Chaque écriture à adresse littérale est consignée avec sa section, son offset et son sens
- [ ] Un accès dont l'adresse est calculée n'y figure pas — la limite est écrite, pas découverte
- [ ] Le bloc survit à l'aller-retour du `.fo`
- [ ] Le refus d'écriture en `"ro"` de l'étage A se comporte à l'identique

## B10 — l'ADR de clôture

**Ce qu'il livre.** Ce qui empêche la prochaine personne à ouvrir le code de lire
un travail à moitié fait. Trois décisions, sur le modèle de l'ADR 0026 qui a
fermé l'étage A :

1. l'ADR 0008 régit l'arithmétique **absolue** ; une valeur relocalisable est
   affine et entière, et `high()` / `low()` en sont la conséquence, pas
   l'exception ;
2. le **fragment** est l'unité placée, et une section sans `org` est ce qui rend
   « relocalisable » définissable sans nouvelle syntaxe ;
3. le format objet est du **texte**, et un format compact est une décision de
   l'étage D.

- [ ] L'ADR est écrit et porte les trois décisions
- [ ] L'amendement à l'ADR 0019 a bien été écrit en B4
- [ ] La correction du §10 sur l'encodeur a bien été écrite en B5
- [ ] `docs/syntax.md` porte `public`, `extern`, `high` et `low`
- [ ] `coutures-de-la-chaine.md` porte la liste de fragments à la place du flux d'octets unique

---

## Ce qui n'est pas dans l'étage B

- **Le placement calculé** — fenêtres, banques, configurations, `ORG` déduit,
  symboles de commutation, concaténation des sections `"ro"`, compression : C1.
- **La vérification** — continuité et ses trois pointeurs, sections miroir,
  `CLOBBERS`, `INIT_FROM` : C2.
- **`BankOf`** comme type de relocalisation — réservé à C1, qui seul connaît les
  banques.
- **Les accès littéraux en lecture et sur les ports** — C2, avec le profil qui
  les interprète.
- **Le nettoyage du backend de snapshot** — affaire du builder.
- **Un format objet compact** — étage D, quand un second producteur existera.
- **`PHASE` / `DEPHASE`** — `org <logique>,<rangement>` fait déjà exactement cela ;
  alias de syntaxe, à décider séparément.
- **`CYCLES_BETWEEN`** — l'étape A6, restée optionnelle et hors du chemin
  critique.
