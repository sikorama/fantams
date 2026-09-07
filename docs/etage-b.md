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
| B2 | Le fragment | B0 | **faite** |
| B3 | La couture du linker | B2 | **faite** |
| B4 | La table des symboles produite par le linker | B3 | **faite** |
| B5 | La relocalisation de bout en bout | B1, B3 | **faite** |
| B6 | `PUBLIC` et `EXTERN` | B5 | **faite** |
| B7 | Le fichier objet, aller-retour | B6 | **faite** |
| B8 | Le multi-objet et l'exemple d'acceptation | B7 | **faite** |
| B9 | Les accès à adresse littérale dans l'objet | B3 | **faite** |
| B10 | L'ADR de clôture | B1, B2, B7, B8, B9 | **faite** |

Deux étapes ne sont pas dans la chaîne : **B1 est parallèle à B0**, et **B9 ne
dépend que de B3**.

Fin de l'étage B : B0 à B10 faites, les sept suites vertes plus celle du linkage,
`docs/syntax.md` à jour, et l'exemple d'acceptation de B8 qui produit le binaire
identique octet pour octet.

**L'étage est fini.** Neuf suites vertes — les sept d'origine, plus `link_test`
et `fo_test` — et le script d'acceptation, tous inscrits dans les **deux** listes
de tests. `examples/separate_a.asm` + `separate_b.asm`, assemblées séparément
puis linkées, produisent le même binaire que `separate_mono.asm`, octet pour
octet.

Une seule chose a changé pour un source d'aujourd'hui : un `org` placé **avant**
une section ne place plus cette section, et un avertissement le dit. Tout le
reste — binaires, `.sym`, `.sna`, diagnostics — est identique à ce qu'il était
avant l'étage.

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

- [x] Les octets vont dans (fragment courant, offset courant), et non plus à une banque dérivée de l'adresse
- [x] Un fragment porte sa propre coverage, allouée à la première écriture
- [x] Une section porte une liste de fragments, chacun connaissant son adresse absolue si un `org` la lui a donnée
- [x] Le plafond de section, `"uninit"`, `BOUNDARY` et `ASSERT_SIZE` se comportent à l'identique
- [x] Les sept suites vertes, **sans une ligne de test modifiée** — c'est la preuve de la migration

Trois choses à savoir sur ce qui a été fait, à relire en B10 :

- **Le placement est rassemblé, pas encore déplacé.** `placeFragments()` rejoue
  les fragments dans leur ordre de création, y dérive les banques et y voit les
  recouvrements. Rejouer dans cet ordre rejoue les écritures dans leur ordre
  d'origine — c'est ce qui laisse les diagnostics de chevauchement identiques.
  Cette fonction est le bloc que **B3 emporte entier** derrière la couture.
- **Un fragment est contigu, croissant, et ne dépasse pas l'espace adressable.**
  Ce qui en sort ouvre un autre fragment. Sans cette règle, un `org` déplacé
  rencontré dans un bloc mesuré donnait un offset négatif.
- **Un seul `ds` ne peut plus dépasser `#10000` octets** — la seule chose que
  l'utilisateur voit changer, et elle n'était pas prévue. Hors `"uninit"`, `ds`
  **émet** ses octets : `ds #7FFFFFF0` en émettait deux milliards qui
  s'écrasaient en silence, et faisait désormais grossir un fragment jusqu'à
  l'épuisement mémoire. La limite est écrite plutôt qu'à découvrir, et vaut
  mieux que le silence d'avant. `docs/syntax.md` la porte.

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

- [x] L'assembleur ne rend plus ni image plate, ni coverage parallèle, ni banques écrites, ni binaire, ni adresse de chargement, ni adresse d'exécution
- [x] L'entrée du programme est un **nom** de symbole, résolu par le linker
- [x] La coverage voyage attachée à ses octets, du fragment jusqu'au bloc placé, jamais en paramètre optionnel parallèle
- [x] Une nouvelle suite teste le linkage à partir d'objets fabriqués à la main
- [x] Cette suite est inscrite dans **les deux** listes de tests du dépôt
- [x] Les 730 assertions de la suite d'assemblage survivent par un helper de harnais qui assemble puis linke ; ce helper n'est pas exposé comme API
- [x] La suite de snapshot verte : la fusion avec une base ne recopie toujours que ce qui est réellement écrit
- [x] Le binaire et le snapshot produits par le CLI sont inchangés

Deux nuances à relire en B10 :

- **`run` accepte une expression, pas seulement un nom.** L'entrée est donc
  `{nom, valeur}` : l'assembleur consigne le NOM quand `run` en porte un — et
  c'est ce nom que le linker résout — la valeur quand il porte autre chose. Ce
  ne sont pas deux mécanismes, c'est une directive qui accepte deux formes.
  L'expression est quand même évaluée par l'assembleur, pour que son refus sorte
  à SA ligne ; un nom qu'il n'a pas résolu ne voyage pas.
- **`warnRunDisplaced` a migré au linker**, et se calcule maintenant sur les
  fragments : « déplacé » veut dire « rangé ailleurs que son adresse logique »,
  et un fragment porte les deux adresses. `displacedRanges_` a disparu.

## B4 — la table des symboles produite par le linker

**Ce qu'il livre.** Dans une section relocalisable, un label n'a pas d'adresse :
la table exportable ne peut plus sortir de l'assembleur. Elle change de maillon
**sans changer de format** — et son consommateur, désassembleur ou émulateur, ne
voit pas la différence. C'est même la raison de le faire ainsi.

- [x] La table est produite par le linker, avec des adresses définitives
- [x] Le format ne change pas d'une colonne ni d'un en-tête
- [x] L'amendement à l'ADR 0019 est écrit **dans cette étape**, pas à la fin de l'étage

Ce qui a bougé sous le capot, à relire en B10 :

- **Un label OUVRE son fragment**, même si aucun octet ne suit : il faut bien
  qu'il habite quelque part, et c'est ce fragment que le linker place. Un
  fragment resté vide ne pose rien.
- **`noteSymbol` tourne aux DEUX passes.** La passe 1 pose la section — c'est
  elle que le contrôle d'écriture en `"ro"` lit, y compris sur une référence
  *avant*, et il tournerait à vide si elle n'arrivait qu'en passe 2. La passe 2
  ajoute le fragment et l'offset, qui n'existent qu'à ce moment-là.
- **`fragmentHere()` est le seul endroit qui décide « quel fragment, quel
  offset »** : les octets comme les labels y passent, ce qui garantit qu'un
  label et l'octet qu'il nomme atterrissent dans le même fragment.

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

- [x] Une section sans `org` est relocalisable et placée par le linker
- [x] `Abs16`, `Rel8`, `High8` et `Low8` sont émises par l'assembleur et résolues par le linker
- [x] Un saut relatif hors de portée est refusé par l'encodeur en intra-section, où il connaît la distance
- [x] Le même, inter-sections, est refusé par le linker, où lui seul la connaît
- [x] `$` se comporte comme un label de la section courante
- [x] La portée hors de [-128, 127] n'est jamais silencieuse : aucun octet de garde n'est émis sans relocalisation
- [x] La correction du §10 est écrite dans cette étape

Quatre décisions prises en cours de route, à relire en B10 :

- **Un `org` AVANT une section ne la place pas**, et c'est le seul cas où un
  source d'aujourd'hui change de sortie. Le §10 affirmait que « toutes les
  sections existantes portent un `org` » ; c'est vrai de celles qui le portent
  *à l'intérieur*, pas de `org #8000` suivi de `section code`. Un tel bloc passe
  de `#8000` à l'adresse choisie par le linker. Ce n'est **jamais silencieux** :
  un avertissement le dit, une fois par section, sur le modèle de la banque
  rémanente de l'ADR 0005 — la lecture est défendable, l'oubli aussi.
- **`ctx.eval()` est le chemin STRICT** et refuse une adresse relocalisable ;
  `evalAddr` et `rel8` sont les deux seuls qui l'acceptent. Le refus est donc le
  défaut, sans avoir à reprendre un par un le numéro de bit, le vecteur `rst`,
  le mode d'interruption et le déplacement indexé.
- **Le prescan.** Quelles sections portent un `org` se décide AVANT les deux
  passes, par un parcours textuel : la première ligne d'une section doit déjà
  savoir si son `pc_` compte en adresses ou en offsets. Les macros et les
  conditionnelles étant déjà déroulées quand l'assembleur voit ces lignes, le
  prescan est exact et non heuristique.
- **Le linker ÉCRIT la valeur d'une relocalisation, il ne l'additionne pas** à
  ce qui se trouve dans les octets. Un objet dont l'addend est faux se lit alors
  à l'œil — ce qui vaudra son prix au `.fo` de B7.
- **Le placement des sections relocalisables suit l'ordre de DÉCLARATION**, après
  le dernier octet absolu. C'est trivial et assumé : c'est ce calcul-là que C1
  remplacera, sans toucher au reste.

L'avertissement du premier point est **volontairement descriptif et non
dissuasif** : laisser le linker placer n'est pas l'usage courant aujourd'hui,
mais rien ne dit que ce ne deviendra pas la norme. Il dit ce qui se passe, il ne
recommande pas d'y renoncer.

## B6 — `PUBLIC` et `EXTERN`

**Ce qu'il livre.** Les deux directives du §4.4, et la portée par défaut qui les
rend utiles. Encore un seul objet à cette étape : le mécanisme existe, rien ne
l'exerce — c'est B8 qui l'exercera.

Un nom ni défini ni déclaré `EXTERN` reste une **erreur d'assemblage**. Un
`EXTERN` implicite transformerait une faute de frappe en relocalisation non
résolue, signalée deux maillons plus loin, alors qu'aujourd'hui l'assembleur la
dit à la bonne ligne.

- [x] Un symbole est local à son objet par défaut
- [x] `PUBLIC` l'exporte, `EXTERN` le déclare défini ailleurs
- [x] Un nom inconnu et non déclaré `EXTERN` est une erreur d'assemblage, à sa ligne
- [x] La mise en forme connaît `public`, `extern`, `high` et `low` — un mot réservé l'est à toutes les phases (ADR 0015)
- [x] Un de ces mots seul en colonne 1 ne reçoit pas de deux-points (la faute d'A4, qui détruisait le source)
- [x] L'idempotence et l'invariant d'octets de la mise en forme tiennent sur une source qui les porte

Ce qui a été fait au-delà de la lettre de l'étape, à relire en B10 :

- **Le linker refuse déjà un `EXTERN` que personne n'exporte**, une fois par
  symbole, en le nommant. C'est une case de B8, mais la laisser ouverte aurait
  livré un `EXTERN` accepté qui écrit des zéros — « accepté et sans effet », le
  seul état que l'étage A a refusé trois fois. B8 y ajoutera le nom de l'objet
  qui le demande, et la double définition.
- **Un `EXTERN` partage l'espace d'identifiants des sections.** Un nom déclaré
  ailleurs vaut une valeur relocalisable exactement comme un label d'une section
  relocalisable — même mécanique, même arithmétique affine, à ceci près que ce
  qui manque est l'adresse d'un SYMBOLE et non la base d'une section. La
  relocalisation porte donc l'un ou l'autre, jamais les deux.
- **Trois refus de portée**, chacun à sa ligne : exporter ce que rien ne
  définit, exporter un nom déclaré `EXTERN`, et déclarer `EXTERN` un nom que cet
  objet définit — dans les deux ordres, puisque c'est la même faute.
- **`high` et `low` deviennent des mots réservés, `hi` et `lo` non.** Les deux
  premiers sont nouveaux, donc rien ne peut casser ; réserver les deux autres
  interdirait un label `hi` qu'un source d'aujourd'hui peut porter.

## B7 — le fichier objet, aller-retour

**Ce qu'il livre.** Un objet qui s'écrit et se relit. Extension `.fo`, format
texte à blocs nommés — sections et fragments, symboles, relocalisations, accès
littéraux, octets en hexadécimal — lisible dans un terminal, comparable par
chaîne dans un test.

Pas de format compact : un objet faux se lit à l'œil, ce qui vaut plus que tout à
l'étage qui introduit la relocalisation, et la couture *format* reste
hypothétique jusqu'à l'étage D.

- [x] Assembler seul écrit un `.fo`
- [x] fantams relit un `.fo`
- [x] L'aller-retour est stable : écrire, relire, réécrire, et les deux textes sont identiques
- [x] Le fichier est lisible et compréhensible sans outil, à l'écran
- [x] Un `.fo` malformé est refusé avec un diagnostic qui nomme la ligne fautive
- [x] La table des symboles exportable en reste un sous-ensemble

Trois choix de format, à relire en B10 :

- **Les octets vont par RUNS partageant leur ligne d'origine** — `data <site>
  <hexa>`, et `gap <n>` pour ce qu'un `ds` a réservé sans l'écrire. Une ligne
  d'objet pour une ligne de source : la provenance voyage sans un nombre par
  octet, et la coverage se lit comme « ce qui n'est pas un `gap` ».
- **Le type de sortie se déduit de l'extension**, comme `.sna` le faisait déjà :
  `-o x.fo` assemble seul, une entrée `.fo` se relit. Pas de drapeau de plus à
  retenir, et c'est le fichier qui dit ce qu'il est.
- **Un `.fo` est UNE unité de compilation.** En écrire un depuis plusieurs
  objets demanderait de renuméroter sections, fragments et sites : ce serait un
  linkage partiel qui ne dit pas son nom, et c'est refusé.

Une nouvelle suite, `fo_test`, inscrite dans les **deux** listes de tests.

## B8 — le multi-objet et l'exemple d'acceptation

**Ce qu'il livre.** La compilation séparée, réellement livrée : N objets linkés en
un binaire, et les diagnostics qui vont avec. Sans cette étape, `EXTERN` serait
un mécanisme qu'aucun test ne peut exercer et `PUBLIC` ne distinguerait rien —
l'état que l'étage A a refusé trois fois.

**Le critère de fin d'étage** est ici.

- [x] N objets sont linkés en un binaire
- [x] Une relocalisation qu'aucun objet ne résout est refusée, en nommant le symbole et l'objet qui le demande
- [x] Deux définitions du même symbole exporté sont refusées, en nommant les deux provenances
- [x] Deux fragments qui se recouvrent sont refusés
- [x] Un exemple d'acceptation dans `examples/` : deux sources portant une section relocalisable, un `PUBLIC` / `EXTERN`, un saut relatif inter-sections et un `high()`
- [x] Assemblées **séparément** puis linkées, elles produisent un binaire **identique octet pour octet** à celui de la version monolithique équivalente

Quatre décisions, à relire en B10 :

- **Le recouvrement change de nature selon qu'il est interne ou non.** À
  l'intérieur d'un fichier il reste un AVERTISSEMENT — réécrire est un idiome,
  et l'auteur voit les deux lignes. Entre deux unités assemblées séparément
  c'est un REFUS : personne ne l'a voulu, et personne ne le verrait. Le refus
  **remplace** l'avertissement — deux diagnostics pour un seul fait en valent
  zéro.
- **Un seul point d'entrée.** Deux `run` sont refusés : en choisir un ferait
  dépendre le point d'entrée de l'ordre des fichiers sur la ligne de commande,
  ce qu'aucun auteur n'a écrit. Même raison pour le double `PUBLIC`.
- **Les sections relocalisables se posent après le dernier octet absolu de TOUS
  les objets**, dans l'ordre où les objets sont donnés puis, à l'intérieur,
  dans l'ordre de déclaration. Les identifiants de section étant locaux à leur
  objet, il y a une table de bases par objet et non une seule.
- **Un source et des objets ne se mélangent pas** sur la ligne de commande. Cela
  marcherait, mais cacherait quel fichier a été réassemblé — or la compilation
  séparée vaut précisément par le fait qu'on SAIT ce qui a été refait.

L'acceptation est un **script**, `tests/accept_separate.sh`, inscrit dans les
deux listes : le fait porte sur le CLI et sur des fichiers, pas sur une
structure en mémoire.

## B9 — les accès à adresse littérale dans l'objet

**Ce qu'il livre.** Le quatrième bloc du §4.6, en **écritures mémoire
seulement** : le sens que le contrôle d'écriture en `"ro"` de l'étage A produit
déjà et teste déjà. Aucun consommateur avant C2 — l'assembleur consigne, il
n'interprète pas.

L'intérêt de le faire ici est de valider le format objet sur un bloc qui n'est ni
des octets ni des symboles. Les lectures et les ports demandent un parcours
d'encodeur que rien ne consomme avant C2 ; les ajouter tard est indolore.

- [x] Chaque écriture à adresse littérale est consignée avec sa section, son offset et son sens
- [x] Un accès dont l'adresse est calculée n'y figure pas — la limite est écrite, pas découverte
- [x] Le bloc survit à l'aller-retour du `.fo`
- [x] Le refus d'écriture en `"ro"` de l'étage A se comporte à l'identique

Deux détails, à relire en B10 :

- **L'offset consigné est celui du CHAMP D'ADRESSE**, pas du premier octet de
  l'instruction — le même que celui de la relocalisation `Abs16` qui l'accompagne
  quand la cible est relocalisable. Dans le `.fo`, les deux lignes se lisent donc
  comme parlant du même champ.
- **L'adresse visée se dit comme une relocalisation** : une base de section plus
  un décalage, un symbole `EXTERN`, ou un nombre tout court quand elle est
  absolue. Une seule grammaire pour « ce qui manque » et « ce qui est visé ».

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

- [x] L'ADR est écrit et porte les trois décisions — `adr/0027-la-relocalisation-et-le-fichier-objet.md`
- [x] L'amendement à l'ADR 0019 a bien été écrit en B4
- [x] La correction du §10 sur l'encodeur a bien été écrite en B5
- [x] `docs/syntax.md` porte `public`, `extern`, `high` et `low`
- [x] `coutures-de-la-chaine.md` porte la liste de fragments à la place du flux d'octets unique

L'ADR reprend, en les tranchant, les notes « à relire en B10 » semées le long de
l'étage. Deux d'entre elles n'y sont pas parce qu'elles appartiennent à un autre
document : l'amendement à l'ADR 0019 vit dans l'ADR 0019, et la correction du
§10 dans la spécification de la chaîne. Une décision écrite à la fin d'un étage
est une décision que personne n'a lue au moment où elle comptait.

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
