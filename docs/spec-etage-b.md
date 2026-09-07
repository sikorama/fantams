# Spécification : étage B — fichier objet et expressions relocalisables

> **Spec d'étage.** Le *quoi* général est au §4 et §10 de
> [spec-chaine-outils.md](spec-chaine-outils.md), le *où* dans
> [coutures-de-la-chaine.md](coutures-de-la-chaine.md). Ce document tranche ce
> que ces deux-là laissaient ouvert, et il **corrige** le §10 sur un point
> (`z80.cpp`) et l'ADR 0019 sur un autre (`--sym`). Le suivi d'étape vit dans
> `etage-b.md`.

## Le problème

L'étage A a donné aux sections un nom, un type, un plafond et une place dans la
table des symboles. Elles ne relogent pas : les adresses restent celles qu'`org`
décide, et un symbole reste un nombre — `expr` rend un `double`, la table des
symboles de l'assembleur est un `map<string, double>`.

Ce qu'un auteur ne peut donc pas faire aujourd'hui :

- **découper un programme en plusieurs unités assemblées séparément.** Le seul
  découpage disponible est l'`include`, c'est-à-dire du préprocesseur : tout
  finit dans une seule passe d'assemblage, et changer une ligne réassemble tout.
- **écrire du code dont il ne choisit pas l'adresse.** Toute section porte un
  `org` parce que rien d'autre ne peut lui donner une adresse. C'est à l'auteur
  de résoudre à la main la question que le §11 attribue au linker : concaténer
  au plus juste des sections `"ro"` pour remplir une ROM de 16 K.
- **référencer un symbole défini ailleurs.** `PUBLIC` et `EXTERN` (§4.4) sont
  écrits dans la spec et n'existent pas.
- **laisser une taille au linkage** (§8). Un blob compressé dont le code a
  besoin de `blob_size` exige aujourd'hui de connaître la taille *pendant*
  l'assemblage — ce qui n'est possible que si la compression remonte dans le
  source, exactement ce que le §2 interdit.

Et pour qui maintient fantams : le travail du linker est **dispersé** dans trois
endroits qui ne savent pas qu'ils le font — la dérivation banque↔adresse, la
limite au-delà de laquelle un dump plat ne sait plus ranger, et les lignes du
CLI qui choisissent la taille du dump. Aucun de ces trois n'est testable pour
lui-même, et l'assembleur rend onze champs dont six sont des décisions de
placement.

## La solution

L'assembleur cesse de rendre une image et rend un **objet** : les quatre blocs
du §4.6, et eux seuls. Une nouvelle couture, le **linker**, prend l'objet et
rend une **image**. En B ce linker est **trivial** — placement absolu tel
qu'`org` le dit, résolution de toutes les relocalisations, concaténation des
fragments — mais il est le seul chemin par lequel un octet sort de fantams, de
sorte qu'une relocalisation fausse fait rougir un test le jour où on l'écrit et
non trois étages plus tard. C1 remplacera son intérieur sans toucher à son
interface.

Ce que l'auteur gagne, concrètement : deux sources assemblées séparément en
`.fo` puis linkées produisent le **même binaire, octet pour octet**, que la
version monolithique équivalente ; une section sans `org` est relocalisable et
son adresse est l'affaire du linker ; `PUBLIC` / `EXTERN` traversent la couture ;
`high()` / `low()` donnent les deux octets d'un symbole qu'on ne connaît pas
encore.

## Histoires

Auteur de source, sauf mention contraire.

1. En tant qu'auteur, je veux assembler un fichier seul en un objet, afin de ne
   pas réassembler tout le programme quand je change une ligne.
2. En tant qu'auteur, je veux linker plusieurs objets en un binaire, afin de
   découper mon programme en unités qui ne se relisent pas l'une l'autre.
3. En tant qu'auteur, je veux qu'un binaire produit en deux temps soit identique
   au binaire produit d'un coup, afin de pouvoir adopter le découpage sans
   craindre d'avoir changé le programme.
4. En tant qu'auteur, je veux déclarer une section sans lui donner d'`org`, afin
   de laisser au linker le soin de la placer.
5. En tant qu'auteur, je veux qu'une section qui porte un `org` continue de se
   comporter exactement comme avant, afin que mes sources existantes assemblent
   sans modification.
6. En tant qu'auteur, je veux pouvoir écrire plusieurs `org` dans une même
   section, afin que le découpage en sections ne m'oblige pas à réorganiser mon
   source.
7. En tant qu'auteur, je veux exporter un symbole avec `PUBLIC`, afin qu'un
   autre objet puisse s'y référer.
8. En tant qu'auteur, je veux déclarer `EXTERN` un symbole défini ailleurs, afin
   d'appeler du code que je n'assemble pas ici.
9. En tant qu'auteur, je veux qu'un symbole soit **local par défaut**, afin que
   deux fichiers puissent employer le même nom de label interne sans se
   heurter.
10. En tant qu'auteur, je veux qu'un nom ni défini ni déclaré `EXTERN` soit une
    **erreur d'assemblage**, afin qu'une faute de frappe me soit dite à la ligne
    où je l'ai écrite, et non par le linker deux maillons plus loin.
11. En tant qu'auteur, je veux qu'un `call` ou un `jp` vers un symbole d'un autre
    objet soit résolu au linkage, afin de ne pas avoir à connaître son adresse.
12. En tant qu'auteur, je veux qu'un `jr` ou un `djnz` vers une cible d'une autre
    section soit résolu au linkage, afin que le découpage n'interdise pas
    l'idiome le plus courant du Z80.
13. En tant qu'auteur, je veux qu'un `jr` hors de portée me soit refusé — par
    l'assembleur quand la distance est intra-section, par le linker quand elle
    ne l'est pas — afin de ne jamais recevoir un déplacement silencieusement
    faux.
14. En tant qu'auteur, je veux `high(x)` et `low(x)`, afin de charger les deux
    octets d'une adresse que je ne connaîtrai qu'au linkage.
15. En tant qu'auteur, je veux que `label >> 8` sur un symbole relocalisable me
    soit refusé **en nommant `high()`**, afin de savoir quoi écrire à la place.
16. En tant qu'auteur, je veux que `label * 2` sur un symbole relocalisable me
    soit refusé, afin de ne pas obtenir un nombre qui n'a pas de sens.
17. En tant qu'auteur, je veux que `fin - debut` reste un nombre quand les deux
    labels partagent leur section, afin de continuer à mesurer la taille d'une
    table.
18. En tant qu'auteur, je veux que `fin - debut` soit refusé quand les deux
    labels sont dans des sections différentes, afin de ne pas écrire une
    hypothèse sur un placement que je n'ai pas choisi.
19. En tant qu'auteur, je veux que `$` se comporte comme un label de la section
    courante, afin que les idiomes qui s'en servent traversent la
    relocalisation.
20. En tant qu'auteur, je veux que `ld bc, blob_size` compile alors que la taille
    n'est pas encore connue, afin que la compression puisse rester une affaire de
    linker (§8).
21. En tant qu'auteur, je veux que le plafond de section, le refus d'écriture en
    `"ro"`, `"uninit"`, `BOUNDARY` et `ASSERT_SIZE` — tout l'étage A — continuent
    de fonctionner à l'identique, afin de ne rien perdre en avançant.
22. En tant qu'auteur, je veux lire mon `.fo` dans un terminal, afin de
    comprendre pourquoi le linkage a échoué sans lancer un outil de plus.
23. En tant qu'auteur, je veux `--sym` inchangé de forme, afin que mon
    désassembleur ou mon émulateur continue de le lire.
24. En tant que consommateur de `--sym`, je veux que les adresses restent des
    adresses **définitives**, afin de ne pas avoir à savoir qu'une section peut
    être relocalisable.
25. En tant qu'auteur, je veux que le linker refuse deux fragments qui se
    recouvrent, afin d'apprendre au linkage ce que l'assembleur m'apprenait au
    sein d'un fichier.
26. En tant qu'auteur, je veux que le linker refuse une relocalisation qu'aucun
    objet ne résout, en nommant le symbole et l'objet qui le demande.
27. En tant qu'auteur, je veux que le linker refuse **deux définitions** du même
    symbole `PUBLIC`, en nommant les deux provenances.
28. En tant qu'auteur, je veux que `beautify` connaisse `public`, `extern`,
    `high` et `low`, afin que la mise en forme ne détruise pas mon source.
29. En tant qu'auteur de sortie `.sna`, je veux que la fusion avec une base
    continue de ne recopier que ce que le source a réellement écrit, afin que la
    coverage survive au passage par l'objet et par l'image (ADR 0012).
30. En tant que mainteneur, je veux tester un chevauchement inter-fragments en
    fabriquant deux structures de dix lignes, afin de ne plus avoir à écrire un
    source Z80 qui le provoque.
31. En tant que mainteneur, je veux qu'aucun test de l'assembleur ne puisse lire
    une adresse absolue dans son résultat, afin qu'aucun test ne se casse le jour
    où un placement change.
32. En tant que mainteneur, je veux que la dérivation banque↔adresse et le choix
    64 K / 128 K vivent derrière le linker, afin qu'ils soient testables pour
    eux-mêmes et qu'il n'y ait qu'un endroit à corriger.
33. En tant que mainteneur, je veux que le préprocesseur reste ignorant des
    sections, afin que la frontière posée par les ADR 0003 et 0005 tienne.
34. En tant qu'étage C1 à venir, je veux une interface de linkage déjà en place
    et déjà testée, afin de n'avoir à remplacer qu'une implémentation.

## Décisions d'implémentation

### D1 — Le fragment est l'unité placée

Une section d'étage A **n'est pas contiguë** : elle peut contenir plusieurs
`org`. Un **fragment** est un bloc d'octets contigu, ouvert par `section` ou par
`org`, portant sa propre coverage, appartenant à une section, et connaissant son
adresse absolue si un `org` lui en a donné une. Une `Section` devient (nom, type,
plafond déclaré, taille, liste de fragments) ; les offsets de relocalisation
restent relatifs à la **section**.

Conséquence, et c'est la définition dont C1 a besoin, obtenue sans nouvelle
syntaxe : **une section sans `org` est un fragment unique, relocalisable ; une
section avec `org` porte un fragment absolu par `org`.**

C'est la généralisation de ce qui existe : le modèle mémoire actuel est déjà
« un bloc d'octets avec sa coverage, alloué à la première écriture, indexé par
un entier dont on ne préjuge pas le sens » (§10). Le fragment est ce bloc dont la
clé cesse d'être une banque. Les trois tables indexées par nom de section que
l'étage A avait laissées en parallèle du modèle mémoire (type, plafond, taille
cumulée) fusionnent dans la `Section` — la dette que l'ADR 0026 avait consignée.

### D2 — Une valeur d'expression porte une section et un coefficient

`expr` cesse de rendre un nombre seul. Le résolveur rend une **valeur** —
`{réel, section, coefficient}` — et le résultat d'évaluation la porte aussi. Une
valeur est **absolue** quand son coefficient est nul, **relocalisable** quand il
vaut 1 sur une section unique.

Le coefficient est borné à `{-1, 0, +1}` et **une seule section est citable à la
fois**. En découlent, calculés dans `expr` et nulle part ailleurs :

- `fin - debut`, même section : absolu (les coefficients s'annulent) ;
- `label` seul : relocalisable ;
- `label * 2`, `label + label` : **refusés** ;
- `label_s1 - label_s2`, sections différentes : **refusé**. Cette expression n'a
  de sens que si son auteur a déjà supposé comment le linker posera les deux
  sections ; la refuser est la politique que l'étage A a tenue trois fois —
  refuser vaut mieux qu'accepter et rendre un nombre sans signification.

L'affinité est une propriété de l'**arbre**, pas du symbole : `label * 2` n'est
détectable qu'en parcourant l'arbre, donc le calcul appartient à `expr` et non à
un appelant qui pré-classerait les symboles.

**L'ADR 0008 n'est pas contredit, il est délimité** : il régit l'arithmétique
**absolue**, où `expr` continue de calculer en réel du début à la fin. Une valeur
relocalisable est **entière par construction** — un coefficient non nul interdit
une partie réelle non entière.

Le préprocesseur, qui tourne avant qu'aucune adresse existe, n'a jamais de valeur
relocalisable à rendre : il remplit un coefficient nul, et rien d'autre ne change
chez lui. C'est l'approfondissement au sens strict : l'interface reste **une
fonction**, l'implémentation grossit.

### D3 — `high()` et `low()`, fonctions explicites

Sous D2, `ld a, label >> 8` et `ld a, label & 255` sont illégaux en section
relocalisable — alors que c'est l'idiome le plus courant du Z80. La réponse est
**`high(x)` et `low(x)`**, qui produisent les relocalisations correspondantes.

Les formes `>> 8` et `& 255` restent légales sur une valeur **absolue**, et sont
refusées sur une valeur relocalisable avec un diagnostic **qui nomme `high()` /
`low()`**. Reconnaître ces deux formes comme des motifs dans l'arbre serait un
piège : `label >> 9` ou `label & 254` ressemblent au motif sans en être, et
l'auteur à qui l'un échoue ne pourrait pas deviner ce qui distinguait l'autre.
C'est mot pour mot la politique de l'ADR 0008 — les besoins entiers se servent
par des fonctions explicites.

Rien ne casse : aucune section existante n'est relocalisable, toutes portent un
`org`.

### D4 — Les quatre types de relocalisation

`Abs16` (une adresse sur deux octets), `Rel8` (un déplacement de saut relatif),
`High8` et `Low8`. `BankOf` est réservé à C1, qui seul connaît les banques. Une
relocalisation porte sa section, son offset, son type, le nom du symbole et un
addend.

La table des accès à adresse littérale (§4.6, bloc 4) est remplie pour les
**écritures mémoire** seulement — le sens que le contrôle d'écriture en `"ro"` de
l'étage A produit déjà et teste déjà. Les `in a,(n)` / `out (n),a` demandent un
parcours d'encodeur que rien ne consomme avant C2 ; les ajouter tard est indolore,
puisque l'assembleur consigne sans interpréter.

### D5 — `z80.cpp` est concerné : correction du §10

Le §10 range l'encodeur parmi les fichiers non concernés par la migration. C'est
faux : il calcule lui-même le déplacement d'un `jr` / `djnz` et refuse un
déplacement hors de [-128, 127]. Il ne peut plus le faire sur une cible
relocalisable.

**L'évaluation offerte à l'encodeur rend donc une valeur**, au sens de D2 :
l'encodeur teste le coefficient, émet un octet de garde et demande une
relocalisation `Rel8`. Le fait — *cette cible n'est pas connue* — appartient à
l'endroit qui l'encode. L'alternative, une interception silencieuse en amont,
rendrait muet le diagnostic le plus utile du Z80.

**La portée reste doublement contrôlée** : l'encodeur pour l'intra-section, où il
connaît la distance ; le linker pour l'inter-section, où lui seul la connaît.

### D6 — L'assembleur rend un objet, le linker rend une image

Les six champs de placement que l'assembleur rend aujourd'hui — l'image plate,
la coverage parallèle, les banques écrites, le binaire, son adresse de
chargement, l'adresse d'exécution — **partent d'un coup**. L'entrée devient un
*nom* de symbole, résolu par le linker.

La coverage **traverse les trois maillons attachée à ses octets** — dans le
fragment puis dans le bloc placé — jamais en paramètre optionnel parallèle. C'est
ce que l'ADR 0012 exige et ce que la signature actuelle du backend de snapshot
laissait à la charge de l'appelant.

Migrent derrière la couture du linker : la dérivation banque↔adresse, la limite
des banques qu'un dump plat sait porter, le choix 64 K / 128 K, la détection de
recouvrement, et la reconstitution de l'image plate que le backend de snapshot
attend. Le CLI cesse d'en dériver aucune.

**Le backend de snapshot n'est pas touché en B.** Sa signature plate est un
problème réel, mais c'est celui du builder ; le mêler à B ferait de l'étage
indivisible un étage à deux sujets.

### D7 — `--sym` est une sortie du linker : amendement à l'ADR 0019

Dans une section relocalisable, un label **n'a pas d'adresse**. La table des
symboles exportable, avec sa valeur, sa banque et son adresse de rangement, ne
peut donc plus sortir de l'assembleur : elle change de maillon. Le **format ne
change pas** d'une colonne, et son consommateur ne voit pas la différence — c'est
même la raison de le faire ainsi.

### D8 — Le format objet est du texte

Extension **`.fo`**, un format texte à blocs nommés : les sections et leurs
fragments, les symboles, les relocalisations, les accès littéraux, et les octets
en hexadécimal. Diffable, lisible dans un terminal, comparable par chaîne dans un
test.

Trois raisons de ne pas le faire compact : un objet faux se lit à l'œil, ce qui
vaut plus que tout à l'étage qui introduit la relocalisation ; les tests
s'écrivent contre des chaînes ; et la couture *format* reste **hypothétique**
jusqu'à l'étage D, qui apportera le vrai second producteur. Pas `.o`, qui
laisserait croire à un objet ELF.

La table `--sym` en reste un sous-ensemble, comme le §10 l'annonçait.

### D9 — La compilation séparée est livrée, pas seulement rendue possible

Le pipeline entier : une source assemblée seule vers un `.fo`, puis N objets
linkés vers un binaire. Sans le multi-objet, `EXTERN` serait un mécanisme
qu'aucun test ne peut exercer et `PUBLIC` ne distinguerait rien — soit exactement
l'état que l'étage A a refusé trois fois : accepté et sans effet, le pire des
états possibles.

Le linker trivial ne coûte pas plus cher à N objets qu'à un : le placement étant
absolu, il concatène les fragments et signale les recouvrements — contrôle qu'il
possède déjà, hérité de la détection de recouvrement de l'assembleur.

### D10 — Portée locale par défaut, pas d'`EXTERN` implicite

Un symbole est local à son objet ; `PUBLIC` l'exporte ; `EXTERN` le déclare
défini ailleurs. Un nom **ni défini ni déclaré `EXTERN` reste une erreur
d'assemblage**. Un `EXTERN` implicite transformerait une faute de frappe en
relocalisation non résolue, signalée par le linker à deux maillons de l'endroit
où elle est écrite — alors qu'aujourd'hui l'assembleur la dit à la bonne ligne.

Ce qui rend ce défaut gratuit : les `include` sont du **préprocesseur**, donc une
source multi-fichiers d'aujourd'hui reste **un seul** objet et ne voit aucune
différence.

Et la leçon de l'étape A4 s'applique telle quelle : `public`, `extern`, `high` et
`low` sont des mots réservés à **toutes les phases** (ADR 0015), donc la mise en
forme doit les connaître, sous peine de traiter un mot seul en colonne 1 comme un
label.

### D11 — L'ordre des étapes est forcé par un invariant

Le dépôt compile et les sept suites sont vertes **à chaque étape**, comme
pendant tout l'étage A.

| # | étape | ce qui reste vert sans y toucher |
|---|-------|----------------------------------|
| **B0** | une seule table de section *(préfacteur)* | les sept suites, sans une ligne de test modifiée |
| **B1** | la valeur d'expression et l'affinité (D2, D3) | les sept suites : l'assembleur rend encore un coefficient nul partout |
| **B2** | le fragment (D1) | les sept suites, **sans une ligne de test modifiée** — c'est la preuve de la migration |
| **B3** | la couture du linker naît (D6) | le backend de snapshot, le préprocesseur, l'encodeur, l'analyseur, la mise en forme |
| **B4** | la table des symboles produite par le linker (D7) | son consommateur, qui ne voit aucune différence |
| **B5** | la relocalisation paie (D4, D5) | — |
| **B6** | `PUBLIC` / `EXTERN` (D10) | — |
| **B7** | le fichier objet, aller-retour (D8) | — |
| **B8** | le multi-objet et l'exemple d'acceptation (D9) | — |
| **B9** | les accès littéraux dans l'objet (D4) | — |
| **B10** | l'ADR de clôture (D12) | — |

Deux étapes sortent de la chaîne : **B1 est parallèle à B0**, et **B9 ne dépend
que de B3**. Le détail des arêtes et l'état de chacune vivent dans
[etage-b.md](etage-b.md).

Ce qui rend l'étage **effectivement divisible**, là où le §10 le dit indivisible :
**la valeur d'expression se teste seule**, avec un résolveur de test rendant des
sections factices, sans un octet d'assembleur. Le morceau qui ne se découpe pas
est B1, et B1 est une étape.

L'ordre n'est pas un choix. Créer la couture d'abord demanderait de définir
l'objet avant de savoir ce qu'une section contient — ce que D1 vient de trancher.
Et une tranche verticale d'abord exigerait D1 à D6 simultanément, c'est-à-dire
l'étage entier en une étape non livrable.

### D12 — Où vont les ADR

Un ADR unique ferme l'étage, à la manière de l'étage A, et porte trois
décisions : **D2 + D3** (l'ADR 0008 régit l'arithmétique absolue ; une valeur
relocalisable est affine et entière), **D1** (le fragment est l'unité placée), et
**D8** (le format objet est du texte, et pourquoi pas compact avant D).

Les deux **amendements** — D7 sur l'ADR 0019 en B4, D5 sur le §10 en B5 — sont
écrits **dans l'étape qui les provoque**, jamais à la fin. Une table des symboles produite par
le linker alors que son ADR dit le contraire est exactement l'écart que l'ADR de
clôture de l'étage A a été écrit pour éviter.

## Décisions de test

**Ce qu'est un bon test ici** : il porte sur un comportement observable à une
couture — des octets, un diagnostic exact, une valeur de symbole, le contenu d'un
objet — et jamais sur la façon dont c'est obtenu. Le harnais du dépôt est
uniforme et sans dépendance : un `main()` qui compte les réussites, des
assertions nommées en clair, un message d'échec qui montre l'attendu et l'obtenu.
On le garde.

**Trois coutures de test, dont deux existent déjà.**

1. **L'évaluateur d'expressions** — couture existante. Toute l'affinité de B1 s'y
   teste avec un résolveur qui rend des sections factices : la soustraction
   intra-section absolue, la soustraction inter-sections refusée, le produit
   refusé, `high()` / `low()`, le décalage refusé sur une valeur relocalisable
   **avec son message exact**. Prior art : la suite d'expressions actuelle, y
   compris ses tests de refus.
2. **L'assemblage** — couture existante, préservée par **un seul helper**. Les 730
   assertions actuelles n'atteignent l'assembleur qu'à travers quatre fonctions
   de harnais ; en B3, ces quatre-là passent par un helper qui assemble **puis**
   linke et rend l'ancienne forme de résultat. Les assertions survivent presque
   inchangées — et une assertion retouchée est une assertion qu'on ne relit pas.
   Ce helper n'est **pas** une API publique : l'exposer figerait dans l'interface
   de l'assembleur les six champs que B3 existe pour en retirer.
3. **Le linkage** — couture **nouvelle**, au point le plus haut : objets
   fabriqués à la main → image. C'est le gain de test décisif : vérifier un
   recouvrement, une relocalisation hors de portée, une définition double ou une
   relocalisation non résolue ne demande plus d'écrire un source Z80 qui la
   provoque, mais deux structures de dix lignes. Nouvelle suite, à inscrire dans
   **les deux** listes de tests du dépôt — l'écart entre elles a déjà été payé
   une fois.

Les tests **nouveaux** de B5 et suivants s'écrivent nativement contre l'objet :
sections, symboles, relocalisations. Après B3, aucun test de l'assembleur ne peut
plus lire une adresse absolue dans son résultat — c'est la garantie qu'aucun test
ne se cassera le jour où un placement changera.

**Le critère de fin d'étage** est un **exemple d'acceptation** : deux sources
portant une section relocalisable, un `PUBLIC` / `EXTERN`, un `jr` inter-sections
et un `high()`, assemblées **séparément** puis linkées, dont le binaire est
identique **octet pour octet** à celui de la version monolithique équivalente.
L'égalité est le seul énoncé qui prouve d'un coup que la relocalisation est
*neutre*, ce qu'aucune suite unitaire ne peut affirmer — et ce qui fera de C1 un
remplacement vérifiable.

La suite de snapshot est le garde-fou de la coverage : elle atteste que ce qui est
seulement réservé ne recouvre pas une base, et elle doit rester verte de bout en
bout de l'étage.

## Hors périmètre

- **Le placement calculé** : fenêtres, banques, configurations, `ORG` déduit,
  symboles de commutation, concaténation des sections `"ro"`, compression. C'est
  C1. Le linker de l'étage B place **absolument**, tel qu'`org` le dit.
- **La vérification** : continuité et ses trois pointeurs, sections miroir,
  `CLOBBERS`, `INIT_FROM`. C'est C2.
- **`BankOf`** comme type de relocalisation : réservé à C1, qui seul connaît les
  banques.
- **Les accès à adresse littérale en lecture et sur les ports** : consignés en
  C2, avec le profil qui les interprète.
- **Le nettoyage du backend de snapshot** : affaire du builder.
- **Un format objet compact** : étage D, quand un second producteur existera.
- **`PHASE` / `DEPHASE`** : un `org <logique>,<rangement>` fait déjà exactement
  cela ; ce serait un alias de syntaxe, à décider séparément.
- **La mesure du temps d'exécution entre deux labels** : autonome, hors du chemin
  critique, et sans rapport avec la relocalisation.

## Notes

**Ce document corrige deux textes existants.** Le §10 de la spec de chaîne, qui
range l'encodeur parmi les fichiers intacts (D5) ; et l'ADR 0019, qui attribue la
table des symboles à l'assembleur (D7). Les deux corrections s'écrivent dans les
étapes B5 et B4.

**Il en amende un troisième par addition** : la structure de section proposée par
le document de coutures porte un flux d'octets unique. D1 y substitue une liste
de fragments, parce qu'une section d'étage A n'est pas contiguë — fait que ni le
§10 ni le document de coutures n'avaient relevé.

**Ce que l'étage B ne change pas, et c'est délibéré** : le préprocesseur reste
ignorant des sections ; l'analyseur et la mise en forme ne voient passer que
quatre mots réservés de plus ; le langage des sources existantes ne bouge pas
d'une virgule — une source d'aujourd'hui porte un `org` dans chaque section, donc
n'a aucune section relocalisable, donc traverse l'étage sans le savoir.
