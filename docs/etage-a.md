# Étage A — suivi

> **Ce fichier tient lieu de ticket.** Une ligne d'état par étape, mise à jour au
> fur et à mesure ; il n'y a rien d'autre à synchroniser. Le *quoi* est au §4 et
> §5 de [spec-chaine-outils.md](spec-chaine-outils.md), le *où* dans
> [coutures-de-la-chaine.md](coutures-de-la-chaine.md) ; ici, l'ordre et l'état.

**Ce qu'est l'étage A**, d'après le §10 : `SECTION` interne, placement toujours
absolu (`org` à l'intérieur), et trois gains — plafond de taille, détection
d'écriture en `"ro"`, section dans la table des symboles. **Sans
relocalisation** : c'est l'étage B qui la paie, et les deux sont indépendants.

**Ce qu'il n'est pas.** Une section, à cet étage, **nomme et classe** ; elle ne
reloge pas. Les adresses restent celles qu'`org` décide, `emit()` continue de
ranger l'octet par banque, et la migration `Space` → `Section` du §10 n'a pas
lieu ici — elle n'est nécessaire qu'au fichier objet, donc à l'étage B. C'est
l'écart assumé entre ce document et la lettre du §10, et la raison en est écrite
dans l'étape **A5**.

L'appel de conception de l'étage est **confirmé** : *A1 compte les octets, il ne
déplace pas le modèle mémoire.* La migration `Space` → `Section` attend l'étage B,
et c'est ce que doit écrire l'ADR de **A5**.

---

## État

| # | Étape | État |
|---|-------|------|
| A0 | `SECTION nom, "type"`, la section dans `--sym`, le refus d'écriture en `"ro"`, `BOUNDARY` | **fait** |
| A1 | Plafond de taille déclaré : `section nom, "type", max` | **fait** |
| A2 | `"uninit"` n'émet pas d'octet | **fait** |
| A3 | `ASSERT_SIZE max` — une sous-zone dans une section | **fait** |
| A4 | `beautify` connaît les nouveaux mots-clés | **fait** |
| A5 | ADR : une section déclarative, et pourquoi la migration attend B | **fait** — [ADR 0026](adr/0026-une-section-nomme-et-classe.md) |
| A6 | `CYCLES_BETWEEN(l1, l2)` (§4.3) — autonome, hors du chemin critique | optionnel |

Fin de l'étage A : A1 à A5 faites, les sept suites vertes, `docs/syntax.md` à
jour, et un `--sym` dont un consommateur peut lire la section. **Atteinte** — sept
suites à 730 assertions. Reste A6, optionnel et hors du chemin critique.

---

## A0 — fait

Ce qui est en place, et par quoi c'est tenu :

- `section nom, "type"` — les trois types du §4.1, le type **obligatoire**, un
  quatrième refusé en nommant les trois, et le type **figé à la première
  déclaration** (rouvrir en `"rw"` ce qui a été déclaré `"ro"` désarmerait le
  contrôle en silence).
- La section qui porte chaque symbole, dans `--sym` : colonne `section`, en-tête
  devenue `name,type,section,value,bank,store,file,line`. Une constante n'en
  porte pas — elle n'habite nulle part, comme le disent déjà `bank` et `store`.
- Le refus statique d'écriture en `"ro"` : `ld (nn),a` et
  `ld (nn),hl/bc/de/sp/ix/iy`, en suivant l'expression (`ld (tbl+1),hl` est
  attrapé). Les lectures et les adresses en dur ne sont pas touchées, et un
  `ld (hl),a` dont `HL` est calculé ne le sera **jamais** (§4.6).
- `BOUNDARY` / `END_BOUNDARY` (§5) : bloc auto-mesuré, la règle unique, le refus
  d'un bloc plus grand que sa frontière, des deux fermetures manquantes et de
  l'imbrication.

Onze tests dans `asm_test.cpp`, sept suites à 693 assertions.

**La dette qu'A0 avait laissée, et qui a mis A1 en tête de liste** : le troisième
argument de `section` était **accepté et ignoré**. Quelqu'un qui écrivait
`section audio, "ro", 0x2000` croyait avoir un plafond et n'en avait aucun — le
pire des trois états possibles, pire que de le refuser. A1 l'a fermée.

---

## A1 — plafond de taille déclaré — fait

Le troisième argument, jusque-là accepté et ignoré, refuse maintenant :

```
Section 'audio' exceeds maximum declared size (0x2140 > 0x2000 bytes)
```

L'erreur est attribuée à la ligne qui **porte** le plafond — la seule que son
auteur puisse corriger — et sort en fin de passe 2 : la taille étant cumulée,
elle n'est connue qu'une fois la dernière réouverture traversée. C'est un `push()`
et non un `structErr()`, qui se tait en passe 2 ; le diagnostic y était avalé,
même piège que pour `ASSERT`.

**Les deux définitions, tranchées.**

1. **La taille est cumulée sur les réouvertures**, et non l'étendue `max − min`
   des adresses : ce qui compte est la place qu'une section demande, celle que le
   linker posera d'un bloc. Avec `org` absolu à l'intérieur, l'étendue serait de
   toute façon un nombre sans signification.
2. **`align` et `boundary` ne comptent pas.** Ils avancent `pc_` sans émettre, et
   à cet étage le remplissage n'existe pas. **À recompter à l'étage C**, où c'est
   le linker qui aligne — d'où l'ADR de A5 plutôt qu'une devinette.

**Une troisième, que le document ne posait pas** et que le premier test a fait
sortir : le plafond est **figé à la première déclaration**, comme le type. Une
réouverture peut l'omettre — c'est la forme normale de l'alternance code /
données — mais ni le relever, ni l'abaisser, ni en introduire un que la première
déclaration ne portait pas. Le relever depuis un fichier inclus désarmerait le
contrôle en silence, exactement comme le ferait un type qui change.

**Où c'est.** Le compteur est dans `emit()` — seul point de passage des octets —
et ne compte qu'en passe 2, hors mesure. `noteSectionMax()` fige le plafond en
passe 1 ; `checkSectionSizes()` constate à la fin. Le modèle mémoire n'a pas
bougé d'une ligne.

**Neuf tests**, au-delà des quatre prévus : le dépassement et son message exact,
la taille exactement égale acceptée, le cumul sur deux réouvertures, l'`align`
qui ne compte pas, le plafond non évaluable en passe 1, les trois cas de
réouverture, et le quatrième argument refusé — accepté et ignoré, il serait le
pire des états, comme l'était le troisième avant A1.

## A2 — `"uninit"` n'émet pas d'octet — fait

Le tableau du §4.1 dit « émet des octets : non » ; c'est maintenant tenu.

**`ds` réserve sans écrire.** `pc_` avance, la coverage ne bouge pas, rien n'entre
dans l'image plate. C'est ce qui laisse intacte la fusion avec une base
(ADR 0012), qui ne recopie que ce qui est couvert — et `sna_test` le confirme.

```asm
        section vars, "uninit"
        org #C000
buffer: ds 16
flag:   ds 1
```

`flag` vaut `0xC010`, le porte dans `--sym` avec sa section, et le binaire n'en
contient pas un octet.

**Le refus est dans `emit()`**, et c'est ce qui rend l'étape petite : `emit()` est
le seul point de passage des octets, donc `db`, `dw`, une chaîne et l'encodeur y
tombent tous sans qu'il faille les reprendre un par un. Le diagnostic ne nomme pas
la directive — `emit()` ne sait pas qui l'appelle, et la ligne citée le dit déjà —
il nomme le **type** de la section, qui est la raison. Dit **une fois par ligne** :
`db "bonjour"` est une faute, pas sept.

**Deux points que le document ne posait pas :**

- **`ds` n'y prend pas de valeur de remplissage.** `ds 16,#FF` laisserait croire à
  une zone initialisée ; le refuser vaut mieux que l'ignorer — c'est la même règle
  que le troisième argument de `section` avant A1.
- **La place réservée compte dans le plafond d'A1.** Le compteur étant dans
  `emit()`, il fallait le rappeler sur le chemin de réservation, sinon une section
  `"uninit"` aurait toujours eu une taille nulle et son plafond n'aurait servi à
  rien. C'est `countSectionBytes()`, appelé des deux côtés.

**Dix tests.** La réservation qui n'émet rien, l'adresse qui avance quand même, ce
qui est réservé qui n'étend pas le binaire, le plafond franchi par de la place
réservée, `db` refusé avec son message exact, `dw` et une instruction refusés, le
refus dit une fois par ligne, la valeur de remplissage refusée, et le non-débord :
ailleurs, `ds` émet toujours son remplissage.

## A3 — `ASSERT_SIZE max` — fait

La zone est un **bloc explicite**, `assert_size n` … `end_assert_size`. « Depuis
le dernier label » se lisait aussi bien, mais un label inséré au milieu changerait
ce qui est mesuré sans que personne l'ait demandé ; et « depuis le début de la
section » n'aurait été que le plafond d'A1 sous un autre nom.

```
Block 'jump_table' exceeds its asserted size (0xA > 0x8 bytes)
```

**Plus petit que prévu**, et pour une raison qui vaut d'être notée : à la
différence de `BOUNDARY`, il n'y a **rien à mesurer d'avance**. `BOUNDARY` doit
connaître la taille du bloc avant de le placer, d'où le double parcours et le
`measuring_`. Ici la zone est assemblée normalement et sa taille est une
différence d'adresses — d'où trois conséquences gratuites :

- **les zones s'imbriquent** (une pile, et non un compteur) : rien ne s'y oppose,
  puisqu'il n'y a pas de mesure préalable qu'un bloc interne arrêterait ;
- **rien ne bouge** : `assert_size` mesure, il n'aligne ni ne remplit ;
- il fallait en revanche **se taire pendant une mesure de `BOUNDARY`**, qui
  repasse sur les lignes du bloc — sinon le contrôle sortait deux fois.

Le premier label de la zone la **nomme** dans le diagnostic, comme celui d'un
`BOUNDARY` ; sans label, elle est désignée par son adresse. L'erreur est attribuée
à la ligne de l'`assert_size`, seule qui porte le nombre à corriger.

**Neuf tests** : le dépassement nommé et le dépassement anonyme, la taille
exactement égale qui n'écarte rien, l'imbrication et la zone interne mesurée pour
elle-même, le contrôle fait une seule fois dans un `BOUNDARY`, la zone jamais
fermée, la fermeture orpheline, et la taille non évaluable en passe 1.

## A4 — `beautify` connaît les nouveaux mots-clés — fait

Deux choses, dont une était un **destructeur de source** : `end_assert_size`, seul
sur sa ligne et en colonne 1, était lu comme un label et recevait un deux-points.
Les enregistrer dans `instructionWords()` a suffi — et c'est exactement ce que
l'ADR 0015 prévoit : un mot réservé l'est à toutes les phases.

Ensuite l'indentation. Les corps de `boundary` et d'`assert_size` prennent leur
cran, la fermeture se rend à celui de son ouvreur. Ce qui a demandé une décision :
`blockKinds()` est la table du **préprocesseur** ; y mêler `BOUNDARY` lui ferait
chercher une structure qu'il n'a pas à connaître — il ne voit ni adresse ni octet.
D'où une table séparée, `asmBlockKinds()`, que seule la mise en forme consulte en
plus de l'autre : un corps de bloc est un corps de bloc, quel que soit l'étage qui
le mesure.

`section` n'y figure pas : elle n'ouvre pas de bloc — pas de fermeture, et son
corps est tout ce qui suit jusqu'à la prochaine. Elle s'indente comme `org`, la
directive qu'elle accompagne.

**Neuf tests**, dont l'idempotence et l'invariant d'octets sur une source qui
porte les deux blocs.

## A5 — l'ADR — fait

[ADR 0026 — À l'étage A, une section nomme et classe ; elle ne
reloge pas](adr/0026-une-section-nomme-et-classe.md). Il porte les quatre
décisions qui étaient dispersées dans ce fichier : pourquoi la migration
`Space` → `Section` attend l'étage B, ce que « taille d'une section » veut dire et
qu'`align` sera à recompter à l'étage C, le type et le plafond figés à la première
déclaration, et `"uninit"` qui n'émet pas.

Il ferme l'étage : sans lui, la prochaine personne à ouvrir `asm.cpp` lirait des
sections qui ne relogent pas et croirait à un travail à moitié fait.

## A6 — `CYCLES_BETWEEN` (optionnel)

§4.3 : l'assembleur connaît le coût en cycles de chaque instruction, c'est donc
le seul endroit où la mesure soit exacte, et son absence est une lacune. Vraiment
autonome — ni section, ni relocalisation, ni modèle mémoire — donc à prendre
quand on veut, y compris avant A1. Hors du chemin critique de l'étage.

À noter : `z80.cpp` encode mais ne dit pas le coût. Il faut donc ajouter la table
des durées, ce qui est du volume vérifiable plutôt que de la conception — et
l'occasion de la vérifier sur machine réelle plutôt que sur documentation.

---

## Ce qui n'est pas dans l'étage A

- **`PUBLIC` / `EXTERN`** (§4.4) — `EXTERN` est une relocalisation, donc B.
- **La table des accès à adresse littérale** (§4.6) — le mécanisme existe déjà,
  c'est le refus d'écriture en `"ro"` qui s'en sert. Mais il n'y a nulle part où
  la mettre avant le fichier objet : B.
- **`PHASE` / `DEPHASE`** (§4.5) — `org <logique>,<rangement>` fait déjà
  exactement ça. Ce serait un alias de syntaxe, pas une fonctionnalité ; à
  décider séparément, sans rapport avec l'étage.
- **`INIT_FROM`, `CLOBBERS`, les sections miroir** (§13.4) — C2.
