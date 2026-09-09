---
status: accepted
---

# Le source peut porter son placement, et il sait ce qu'il perd

Une déclaration de section peut dire **où elle va**, dans le vocabulaire du
profil : `section gfx1, "ro" IN w1 OF ext_w1<1>`. Le linker la place et lui rend
ses valeurs de commutation, sans qu'aucun script de lien existe.

C'est une **rupture délibérée du §11**, qui dit que le source est agnostique. Cet
ADR l'énonce, dit ce qu'un tel source perd, et pourquoi les deux autres façons de
placer restent entières à côté.

## Contexte

Il y a trois façons de placer une section, et elles existaient déjà à deux :

- **le script de lien** (étage C1). `aliased.asm` ne nomme aucun emplacement ;
  `aliased.ld` dit où chaque section vit. Le source est réutilisable tel quel
  pour une autre machine — on change le script, pas le programme ;
- **`org b<n>:` à la main** (ADR 0005). `aliased_org.asm` place tout lui-même :
  un préfixe de banque par bloc, et les valeurs de commutation en `equ`. C'est la
  forme que prend un source venu d'un autre assembleur, et la seule utilisable
  quand l'hôte ne sait pas éditer de script.

La seconde paie un prix que son propre en-tête nomme : **chaque banque y est
nommée deux fois**, une fois dans l'`org b<n>:` et une fois dans l'`equ`, et rien
ne garantit qu'elles restent d'accord. Un auteur qui déplace un bloc d'une banque
à l'autre doit se souvenir de changer la constante de commutation ; s'il l'oublie,
rien ne le lui dit — la machine ne plante pas au basculement, mais trois
instructions plus loin.

Et le §11 n'est pas une règle absolue : `aliased_org.asm` existe, il est
maintenu, et son en-tête dit déjà **« un placement peut être une propriété du
PROGRAMME »**. Ce qui manquait n'était pas la permission de placer dans le
source : c'était une façon de le faire **sans recopier ce que le profil sait
déjà**.

## Décision 1 — La section nomme une CONFIGURATION, jamais une banque

`IN ext_w1<1>` nomme un **état de carte** du profil. `IN ext1` — la banque —
serait plus court et paraît plus direct ; il est refusé, et pour une raison de
fait, pas de goût.

Sur `cpc6128`, la banque `ext1` est amenée dans `w1` par **deux** états :

| état | valeur de commutation |
|---|---|
| `ext_w1<1>` | `&C5` |
| `all_ext`   | `&C2` |

Une clé par banque vaudrait donc deux choses, et la règle qui gouverne les
symboles du linker — un symbole qui vaudrait deux choses selon l'ordre de lecture
est pire qu'un symbole absent — le ferait disparaître. **Un état et son argument
désignent une carte, et une carte donne une valeur.** C'est aussi ce qui permet
au source qui se place lui-même d'obtenir `__val_ram_ext_w1_1` : cette clé se
dérive du profil seul, sans script.

Le corollaire est que le vocabulaire du placement est **celui du profil**, et
non celui du matériel nu. Un numéro de banque est un fait de la machine ; un état
de carte est ce que le profil a choisi de nommer, et c'est le seul niveau où une
valeur de commutation existe.

## Décision 2 — Deux graphies, et la courte n'est licite que sans ambiguïté

    section gfx1, "ro" IN ext_w1<1>          ; courte
    section gfx1, "ro" IN w1 OF ext_w1<1>    ; verbeuse, toujours valide

La forme courte vaut quand la configuration ne mappe **qu'une** fenêtre : elle
n'est alors ambiguë pour personne. Quand elle en mappe plusieurs, elle est
refusée **en les nommant** et en montrant la forme qui tranche — plutôt que d'en
choisir une, ce qui placerait la section quelque part sans que l'auteur l'ait dit.

**Ce que le profil livré en fait.** Sur `cpc6128`, un état décrit *toute* la
carte — `ext_w1<b>` s'écrit `{ w0 base0  w1 ext<b>  w2 base2  w3 base3 }` — et non
la seule fenêtre qui change. La forme courte y est donc refusée presque partout,
et la verbeuse est la forme normale. Ce n'est pas un défaut de la règle : c'est
elle qui s'applique, et le refus est instructif. La forme courte sert les
configurations qui ne mappent réellement qu'une fenêtre, comme
`rom_lower.on { w0 rom_lo }`.

## Décision 3 — Le script SURCHARGE le source, avec un avertissement

Quand un script place une section que le source place aussi, **le script gagne**,
et deux avertissements le disent — l'un sur la déclaration de section, l'autre sur
la ligne du script qui l'emporte.

Le refus aurait été plus simple, et il a été écarté : reprendre un source dont on
ne veut pas éditer les sections est un usage réel, et l'interdire ferait du
placement en source une décision irrévocable. Mais la surcharge ne peut pas être
**muette**, et le piège n'est pas cosmétique : un source qui se place lui-même
commute avec la valeur de **sa** configuration, qui est la mauvaise dès que le
script l'a posé ailleurs. La faute est indétectable à la lecture des deux fichiers
pris séparément, et ne se voit qu'à l'exécution.

## Décision 4 — `org b<n>:` reste, et n'est pas déprécié

L'ADR 0005 n'est pas amendé. `org b<n>:` garde exactement ce qu'il est : une
notation d'**émission directe**, qui range des octets dans une banque nommée par
son numéro, **sans linker**. Elle reste la forme d'accueil d'un source venu d'un
autre assembleur, et la seule disponible quand aucun profil n'est donné.

Les deux ne se recouvrent pas, et ne doivent pas être mêlées : **un `org` préfixé
d'une banque à l'intérieur d'une section placée par `IN` est refusé**. `IN` a déjà
dit où la section va ; le préfixe le redit, et rien ne garantirait qu'ils restent
d'accord — c'est la faute même que ce placement existe pour éviter.

Un `org` **nu** dans une section placée reste licite, et y vaut un **décalage** :
la section est relocalisable, son `pc` compte en offsets, et `org 0x100` dit « ce
qui suit commence à 0x100 de la base que le linker donnera ».

## Ce qu'un tel source perd

**Il est couplé à la machine.** `ext_w1<1>` est un nom du profil `cpc6128` ; un
source qui l'écrit ne se construit pas pour une autre cible sans être édité, là où
`aliased.asm` change de machine en changeant de script. C'est le §11 qui est
rompu, et il l'est en connaissance de cause.

**Ce qu'il gagne en échange** : une seule nomination au lieu de deux. La banque,
l'adresse logique et la valeur de commutation viennent toutes du même mot, et
déplacer une section d'une configuration à l'autre est une ligne — le linker en
tire la nouvelle valeur tout seul. `examples/aliased_sym.asm` ne contient ni un
`org`, ni un `equ`, ni un nombre de commutation écrit à la main.

**Et l'échange est vérifiable, pas affirmé.** `tests/accept_aliased.sh` compare
les **trois** chemins octet pour octet : le script, la main, les sections. Aucun
oracle extérieur — trois chemins du même outil, et leur accord fait la preuve.

## L'ADR 0005 et l'ADR 0026, relus

**ADR 0005 — inchangé.** Sa portée était déjà énoncée : le préfixe qualifie une
adresse « dans les directives de placement ». Il continue de le faire, sur le
chemin sans linker. Sa raison d'être — *« l'adresse logique doit être écrite,
jamais déduite »*, parce que les sept sources du corpus qui emploient `BANK`
écrivent toutes `org #4000` pour les banques 4 à 7 — est même **confirmée** par
ce placement-ci : la fenêtre donne l'adresse logique, et elle ne se déduit pas du
numéro de banque.

**ADR 0026 — sa dette est éteinte, et sa lettre tient.** Il disait qu'à l'étage A
« une section nomme et classe ; elle ne reloge pas », et annonçait la migration
pour l'étage B. Elle a eu lieu. Ce que cet ADR-ci ajoute ne contredit rien de
0026 : la section continue de nommer et de classer, et ce qu'elle porte en plus
est une **chaîne que l'assembleur ne lit pas**. `emit()` reste le seul point de
passage des octets, et l'assembleur ne connaît toujours aucune machine.

## Conséquences

- **L'assembleur ne connaît aucune machine, et le §1 tient.** Il porte
  `ext_w1<1>` sans l'interpréter, comme il reçoit déjà des nombres par
  `switchSymbols`. Toutes les résolutions — configuration inconnue, fenêtre non
  mappée, forme courte ambiguë — sont des refus du **linker**, qui lit le profil.
- **Il n'y a qu'un moteur de placement.** `IN` construit une **carte**, la même
  structure qu'un `.ld` produit, et la verse dans celle du script avant que quoi
  que ce soit ne place. Le chevauchement, le mou chiffré, l'`ORG` déduit et le
  découpage s'appliquent sans savoir d'où elle vient — et c'est le contrôle qui
  prouve que `IN` est une seconde syntaxe d'entrée et non un second moteur.
- **Un seul analyseur nomme une configuration.** `script::parseConfigRef` est
  exposée, et le source passe par elle. Deux analyseurs auraient fait deux
  langages qui se ressemblent, et la ressemblance aurait fini par se défaire.
- **Le placement est figé à la première déclaration**, comme le type et le
  plafond. Une réouverture muette le garde ; une réouverture qui le change, ou qui
  en **introduit** un, est refusée — il vaudrait rétroactivement pour les octets
  déjà posés.
- **Les clés de commutation PAR SECTION restent réservées au script.**
  `__val_ram_gfx1` demande de savoir quelle configuration voit `gfx1` ; avec `IN`
  le source le dit, mais le calcul tourne **avant** l'assemblage et ne peut pas le
  lire. Offrir cette clé au linkage et pas à l'assemblage ferait d'un même nom
  deux langages selon la façon dont on compile. Un source qui se place lui-même
  nomme sa configuration — `__val_ram_ext_w1_1` —, et ce nom vaut par les deux
  chemins.
- **Le format objet monte à la version 2.** Il porte deux clés de plus, et une
  section placée par le source s'y écrit `reloc` bien qu'elle porte un `org`. Un
  fantams d'avant lirait ces objets sans broncher et lierait la section à une
  adresse fausse en silence : c'est la classe de changement que ce champ existe
  pour attraper.
