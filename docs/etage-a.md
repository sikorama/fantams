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
| A2 | `"uninit"` n'émet pas d'octet | à faire |
| A3 | `ASSERT_SIZE max` — une sous-zone dans une section | à faire — *forme arrêtée : un bloc explicite* |
| A4 | `beautify` connaît les nouveaux mots-clés | à faire |
| A5 | ADR : une section déclarative, et pourquoi la migration attend B | à faire |
| A6 | `CYCLES_BETWEEN(l1, l2)` (§4.3) — autonome, hors du chemin critique | optionnel |

Fin de l'étage A : A1 à A5 faites, les sept suites vertes, `docs/syntax.md` à
jour, et un `--sym` dont un consommateur peut lire la section.

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

## A2 — `"uninit"` n'émet pas d'octet

**Ce que ça donne.** Le tableau du §4.1 dit « émet des octets : non » pour
`"uninit"`. Aujourd'hui rien ne l'empêche, et une section réservée peut partir
avec des octets dedans — que le linker n'aurait aucun endroit où mettre.

**Le point à trancher** : `ds` dans une section `"uninit"` est exactement son
usage — réserver. Mais `emitDS` passe par `emit()`, donc il *écrit* des octets de
remplissage. Il faut donc que `ds` **réserve sans émettre** quand la section est
`"uninit"` : `pc_` avance, la coverage ne bouge pas. Et `db` / `dw` / une
instruction y sont refusés, en nommant le type de la section.

C'est la seule étape de A qui touche un chemin d'émission existant. Elle vient
après A1 pour une raison désormais concrète : le compteur d'A1 est **dans
`emit()`**. Si `ds` cesse d'y passer, il faut compter la réservation ailleurs,
sans quoi une section `"uninit"` aurait toujours une taille nulle et son plafond
ne servirait à rien. C'est le premier test à écrire : un plafond dépassé par des
octets réservés.

**Coût** : petit à moyen. Trois cycles, mais le premier touche `emitDS` et la
coverage, dont dépend la fusion avec une base (ADR 0012) : la suite `sna_test`
est le garde-fou à surveiller.

---

## A3 — `ASSERT_SIZE max`

**Ce que ça donne.** Le §4.1 : « vérifie de la même façon une sous-zone à
l'intérieur d'une section — une table de saut, par exemple ».

**La forme est arrêtée : un bloc explicite**, sur le modèle de `BOUNDARY` /
`END_BOUNDARY`. La spec ne disait pas *d'où* la sous-zone commence ; les trois
lectures possibles, et la raison du choix :

- **depuis le dernier label** — `ASSERT_SIZE` mesure du label précédent à la
  ligne où elle est écrite. Rien à ouvrir, rien à fermer, et ça se lit ; mais la
  zone est implicite, et un label inséré au milieu change ce qui est mesuré sans
  que personne l'ait demandé.
- **un bloc explicite**, sur le modèle de `BOUNDARY` / `END_BOUNDARY`, déjà écrit
  et déjà testé. Cohérent avec ce qui existe, au prix d'un mot-clé de plus.
- **depuis le début de la section** — alors ce n'est plus une sous-zone, c'est le
  plafond de A1 sous un autre nom, et l'étape n'existe pas.

`BOUNDARY` a déjà établi la forme du bloc auto-mesuré dans ce langage, et une
zone explicite ne se déplace pas sous les pieds de son auteur — c'est ce qui
tranche contre la première lecture, à un mot-clé près.

**Coût** : petit une fois la forme choisie — la mesure est celle de `BOUNDARY`,
qui existe.

---

## A4 — `beautify` connaît les nouveaux mots-clés

**Ce que ça donne.** `--beautify` indente le corps d'un bloc d'un cran. Il ne sait
pas que `BOUNDARY` / `END_BOUNDARY` en est un, ni que `SECTION` commence en
colonne 1 ou non. Une source qui les utilise ressort donc mal mise en forme, et
l'invariant de l'ADR 0017 — aucune source ne doit être assemblable seulement
après passage par la mise en forme — ne dit rien sur l'inverse.

**Couture.** `beautify::format`, testée dans `beautify_test.cpp`. C'est la seule
étape de A hors de `asmb::assemble`.

**Coût** : petit. Deux cycles, plus l'invariant d'octets que `beautify_test`
vérifie déjà.

---

## A5 — l'ADR

**Ce que ça donne.** Une décision écrite là où on la cherchera : **une section, à
l'étage A, nomme et classe ; elle ne reloge pas.** Ce que l'ADR doit porter, et
qui est aujourd'hui dispersé dans ce fichier :

- pourquoi la migration `Space` → `Section` du §10 **attend l'étage B** : elle
  n'est nécessaire qu'au fichier objet, et les trois gains de A s'obtiennent sans
  elle ;
- ce que « taille d'une section » veut dire (A1, les deux définitions), et le fait
  que `align` devra être recompté à l'étage C ;
- que le type **et le plafond** sont figés à la première déclaration, et pourquoi
  (le refus silencieux qu'on éviterait sinon).

Il ferme l'étage : sans lui, la prochaine personne à ouvrir `asm.cpp` lira des
sections qui ne relogent pas et croira à un travail à moitié fait.

---

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
