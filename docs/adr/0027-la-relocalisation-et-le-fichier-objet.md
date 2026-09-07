---
status: accepted
---

# À l'étage B, une expression est affine, un fragment est l'unité placée, et l'objet est du texte

L'étage B a donné à fantams la compilation séparée : l'assembleur rend un
**objet**, le linker rend une **image**, une section sans `org` est placée par le
linker, et `PUBLIC` / `EXTERN` traversent la couture. Trois décisions le
structurent, et cet ADR les fixe pour que la prochaine personne à ouvrir le code
ne lise pas un travail à moitié fait.

## Contexte

À la fin de l'étage A, un symbole était un nombre : `expr::Resolver` rendait un
`double`, la table des symboles de l'assembleur était un `map<string, double>`,
et toute section portait un `org` parce que rien d'autre ne pouvait lui donner
une adresse. Le travail de linker existait, mais **dispersé** dans trois endroits
qui ne savaient pas qu'ils le faisaient : la dérivation banque↔adresse, la limite
au-delà de laquelle un dump plat ne sait plus ranger, et les lignes du CLI qui
choisissaient la taille du dump.

## Décision 1 — L'ADR 0008 régit l'arithmétique ABSOLUE

L'ADR 0008 dit que fantams calcule en réel du début à la fin, et que les besoins
entiers se servent par des **fonctions explicites** plutôt que par des règles
devinées. Cet ADR ne le contredit pas : il le **délimite**.

Une valeur d'expression porte désormais `{réel, section, coefficient, octet
retenu}`. Elle est **absolue** quand son coefficient est nul — et là, l'ADR 0008
s'applique mot pour mot, rien n'a changé. Elle est **relocalisable** quand le
coefficient vaut 1 sur une section unique : elle vaut « la base de cette section,
plus un décalage ».

Une valeur relocalisable est **affine et entière par construction**. Le
coefficient est borné à `{-1, 0, +1}` et une seule section est citable à la fois.
En découlent, calculés dans `expr` et nulle part ailleurs :

- `fin - debut`, même section : absolu — les coefficients s'annulent, et c'est
  ainsi qu'une table continue de se mesurer ;
- `label` seul : relocalisable ;
- `label * 2`, `label + label` : refusés ;
- `label_s1 - label_s2`, sections différentes : refusé — cette expression n'a de
  sens que si son auteur a déjà supposé comment le linker posera les deux
  sections ;
- une partie fractionnaire sur une valeur relocalisable : refusée. Une adresse
  est un entier.

**L'affinité est une propriété de l'ARBRE, pas du symbole.** `label * 2` n'est
détectable qu'en le parcourant, donc le calcul appartient à `expr` et non à un
appelant qui pré-classerait les symboles.

**`high()` et `low()` sont la CONSÉQUENCE de cette règle, pas son exception.**
Sous elle, `label >> 8` et `label & 255` — l'idiome le plus courant du Z80 —
deviennent illégaux en section relocalisable. La réponse est deux fonctions
explicites, exactement la politique de l'ADR 0008. Les formes `>> 8` et `& 255`
restent légales sur une valeur absolue et sont refusées sur une valeur
relocalisable **avec un diagnostic qui nomme `high()` / `low()`**.

Reconnaître ces deux formes comme des motifs dans l'arbre aurait été un piège :
`label >> 9` et `label & 254` leur ressemblent sans en être, et l'auteur à qui
l'un échoue ne pourrait pas deviner ce qui distinguait l'autre.

**Conséquences.** Dans `expr`, le refus est le comportement **par défaut** :
seuls `+`, `-`, `high()` et `low()` acceptent une valeur relocalisable, tout le
reste passe par `known()`. Un opérateur ajouté demain sera donc refusé sur une
adresse tant que personne n'aura écrit ce qu'il en fait. Même dessin côté
encodeur : `ctx.eval()` refuse, `evalAddr` et `rel8` acceptent, ce qui couvre
sans les reprendre un par un le numéro de bit, le vecteur `rst`, le mode
d'interruption et le déplacement indexé.

`hi()` et `lo()` sont des **graphies** de `high()` et `low()`, valeur
relocalisable comprise : deux fonctions dont une seule accepterait une adresse
serait une asymétrie qu'aucune règle ne fait deviner. Elles ne deviennent pas des
mots réservés, elles — `high` et `low` sont nouveaux, donc rien ne peut casser ;
réserver `hi` interdirait un label qu'un source d'aujourd'hui peut porter.

## Décision 2 — Le FRAGMENT est l'unité placée

Une section n'est **pas contiguë** : elle peut contenir plusieurs `org`. Un
**fragment** est un bloc d'octets contigu, ouvert par `section` ou par `org`,
portant sa propre coverage, appartenant à une section, et connaissant son adresse
de rangement si un `org` la lui a donnée. C'est lui que le linker place.

C'est la généralisation du modèle mémoire de l'ADR 0006 — « un bloc d'octets avec
sa coverage, alloué à la première écriture, indexé par un entier dont on ne
préjuge pas le sens ». Le fragment est ce bloc dont la clé cesse d'être une
banque.

**Ce qui en découle, et c'est la raison de le faire ainsi : une section sans
`org` est relocalisable.** « Relocalisable » devient définissable **sans nouvelle
syntaxe** — il n'y a pas de mot-clé à ajouter, pas d'attribut de section à
inventer, et une source qui porte un `org` dans chaque section se comporte
exactement comme avant.

Trois règles tiennent le fragment :

1. il est **contigu, croissant, et ne dépasse pas l'espace adressable** ; ce qui
   sort de là ouvre un autre fragment ;
2. **un seul endroit** décide « quel fragment, quel offset » — `fragmentHere()` —
   et les octets comme les labels y passent, ce qui garantit qu'un label et
   l'octet qu'il nomme atterrissent dans le même fragment ;
3. **la coverage voyage avec ses octets** : `Fragment::prov` est parallèle à
   `bytes` dans le même objet et porte les deux faits à la fois — zéro = trou
   réservé, non nul = écrit par cette ligne. C'est ce que l'ADR 0012 exigeait et
   que la signature du backend de snapshot laissait à la charge de l'appelant.

**Un `org` AVANT une section ne la place pas**, et c'est le seul cas où un source
d'aujourd'hui change de sortie. Ce n'est jamais silencieux : un avertissement le
dit, une fois par section, sur le modèle exact de la banque rémanente de l'ADR
0005 — la lecture est défendable, l'oubli aussi. Il est délibérément
**descriptif et non dissuasif** : laisser le linker placer n'est pas l'usage
courant aujourd'hui, mais rien ne dit que ce ne deviendra pas la norme.

**Le placement de l'étage B est trivial et assumé** : les sections que personne
n'a placées se suivent après le dernier octet absolu de tous les objets, dans
l'ordre où les objets sont donnés puis, à l'intérieur, dans l'ordre de
déclaration. C'est ce calcul-là — `placeRelocSections()` — que C1 remplacera,
sans toucher au reste.

## Décision 3 — Le format objet est du TEXTE, et le compact est une décision de l'étage D

Le `.fo` est un format texte à blocs nommés : les sections et leurs fragments,
les symboles, les relocalisations, les accès à adresse littérale, et les octets
en hexadécimal.

Trois raisons de ne pas le faire compact :

1. **un objet faux se lit à l'œil**, ce qui vaut plus que tout à l'étage qui
   introduit la relocalisation — quand un octet atterrit au mauvais endroit, la
   question est toujours « qu'est-ce que l'objet disait ? » ;
2. **les tests s'écrivent contre des chaînes** : l'aller-retour — écrire, relire,
   réécrire — se vérifie par une comparaison de texte, le seul contrôle qui
   attrape à la fois un champ qu'on n'écrit pas et un champ qu'on ne relit pas ;
3. **la couture *format* reste hypothétique** jusqu'à l'étage D, qui apportera le
   vrai second producteur. Un format compact aujourd'hui optimiserait une
   contrainte que personne n'a mesurée.

Pas `.o`, qui laisserait croire à un objet ELF.

Deux conséquences de forme. Les octets vont par **runs partageant leur ligne
d'origine** — `data <site> <hexa>`, et `gap <n>` pour ce qu'un `ds` a réservé
sans l'écrire : une ligne d'objet pour une ligne de source, la provenance voyage
sans un nombre par octet, et la coverage se lit comme « ce qui n'est pas un
`gap` ». Et **un `.fo` est UNE unité de compilation** : en écrire un depuis
plusieurs objets demanderait de renuméroter sections, fragments et sites, ce qui
serait un linkage partiel qui ne dit pas son nom.

**Un format compact est une décision de l'étage D**, à prendre quand un second
producteur existera et qu'on saura ce qu'il coûte.

## Ce que cet ADR ne décide pas

Le **placement calculé** — fenêtres, banques, configurations, `ORG` déduit,
concaténation des sections `"ro"`, compression — est C1. La **vérification** —
continuité, sections miroir, `CLOBBERS`, `INIT_FROM` — est C2. `BankOf` comme
type de relocalisation est réservé à C1, qui seul connaîtra les banques. Les
accès littéraux **en lecture et sur les ports** sont C2, avec le profil qui les
interprète. Le nettoyage du backend de snapshot est l'affaire du builder : sa
signature plate est une dette réelle, mais la mêler à l'étage B en aurait fait un
étage à deux sujets.

## Deux amendements, écrits dans l'étape qui les provoquait

Ils ne sont pas ici, et c'est voulu — une décision écrite à la fin d'un étage est
une décision que personne n'a lue au moment où elle comptait :

- **l'ADR 0019** est amendé dans son propre fichier (étape B4) : la table des
  symboles est produite par le LINKER, avec des adresses définitives, et son
  format ne change pas d'une colonne ;
- **le §10 de la spécification** est corrigé dans `spec-chaine-outils.md` (étape
  B5) : `z80.cpp` **est** concerné par la relocalisation, parce qu'il calcule
  lui-même le déplacement d'un `jr` et refuse ce qui sort de [-128, 127].

## Conséquences

Le refus est le comportement par défaut partout où une adresse peut manquer, ce
qui rend l'ajout d'un opérateur ou d'un contexte d'opérande sûr par construction.

La portée d'un saut relatif reste **doublement contrôlée** : l'encodeur pour
l'intra-section, où il connaît la distance ; le linker pour l'inter-section, où
lui seul la connaît. L'octet de garde n'est jamais émis sans sa relocalisation,
donc une portée hors bornes ne devient jamais silencieuse.

Le recouvrement **change de nature** selon qu'il est interne ou non. À
l'intérieur d'un fichier il reste un avertissement — réécrire est un idiome, et
l'auteur voit les deux lignes. Entre deux unités assemblées séparément c'est un
refus : personne ne l'a voulu, et personne ne le verrait.

Une nouvelle couture testable est née. Vérifier un recouvrement inter-objets, une
relocalisation hors de portée, une définition double ou une relocalisation non
résolue ne demande plus d'écrire un source Z80 qui la provoque, mais deux
structures de dix lignes.
