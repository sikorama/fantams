# Spécification : étage C1 — le linker qui place et calcule

> **Spec d'étage.** Le *quoi* général est aux §6, §7, §9, §12 et §13 de
> [spec-chaine-outils.md](spec-chaine-outils.md), le *où* dans
> [coutures-de-la-chaine.md](coutures-de-la-chaine.md). Ce document tranche ce
> que ces cinq-là laissaient ouvert, et il **corrige** le §10 sur un point (le
> porteur du profil) et l'ADR 0005 sur son statut. Le suivi d'étape vit dans
> `etage-c1.md`.

## Le problème

L'étage B a donné à fantams la compilation séparée : l'assembleur rend un objet,
le linker rend une image, une section sans `org` est placée par le linker, et
`PUBLIC` / `EXTERN` traversent la couture.

Mais **ce linker place absolument, et ne calcule rien.** `placeRelocSections()`
est un curseur : les sections que personne n'a placées se suivent après le
dernier octet absolu, dans l'ordre des objets. Il ne connaît ni fenêtre, ni
banque, ni configuration.

Ce qu'un auteur ne peut donc pas faire aujourd'hui :

- **écrire un programme banqué sans nommer les banques dans sa source.** La
  seule façon de ranger un octet en RAM étendue est `org b<n>:adresse` — l'ADR
  0005, avec sa rémanence et son masquage. Le §12.2 montre le renversement que
  C1 apporte : la source nomme une **section**, la configuration dit dans quelle
  banque elle vit et la fenêtre lui donne son `ORG`. Déplacer le player ne
  touche plus une ligne de source.
- **laisser le linker remplir une ROM au plus juste.** Le §11 lui attribue ce
  travail — concaténer les sections `"ro"` de dix fichiers dans 16 K — et il ne
  peut pas le faire : une section de même nom dans deux objets reçoit
  aujourd'hui **deux bases disjointes**.
- **écrire une valeur de commutation sans la coder en dur.** Le §12.3 promet
  `__port_ram_audio` et `__val_ram_audio` ; ils n'existent pas. Une valeur
  écrite en dur devient fausse **en silence** le jour où la section bouge.
- **décrire sa machine.** Le §13 fait de « écrire un profil est un fichier de
  données » le seul test qui prouve que le découpage a servi. Rien ne le porte.

Et pour qui maintient fantams : le §7 énumère quatorze natures d'information
qu'un profil doit savoir dire, et **aucune structure ne les porte**. Les valeurs
vérifiées dorment dans `docs/recherche/`, où rien ne les exécute.

## La solution

Le linker reçoit deux entrées de plus — un **profil de cible** et un **script de
linkage** — et son intérieur cesse d'être un curseur. Il calcule : la fenêtre
donne l'`ORG`, la configuration donne la banque, les sections de même nom se
fusionnent, les chevauchements se refusent, le mou se chiffre, et les valeurs de
commutation deviennent des symboles.

`Image` ne gagne pas un champ. Aucun appelant ne change sa façon de lire le
résultat. C'est ce que l'étage B avait promis, et c'est ce qui rend C1 livrable
sans promettre C2.

Ce que l'auteur gagne, concrètement : l'exemple du §12.2 s'assemble, se linke et
s'exporte ; un `org` disparaît de la source au profit d'une ligne de script ;
`ld bc, __port_ram_audio | __val_ram_audio` se résout ; et `--dump-profile
cpc6128` rend le texte à partir duquel modifier sa propre machine.

## Histoires

Auteur de source, sauf mention contraire.

**Je place mon player audio dans une banque étendue sans l'écrire dans ma
source.** J'écris `SECTION audio, "ro"` et, dans le script, `CONFIG ext_w1<1> {
w1 { SECTION audio } }`. Mes labels valent `&4000 + offset` parce que la
configuration dit que `ext1` y apparaît.

**Je déplace ce player dans une autre banque sans toucher à ma source.** Je
change `ext_w1<1>` en `ext_w1<2>` dans le script. Les adresses logiques sont les
mêmes, la banque de rangement change, et les symboles de commutation suivent.

**Je commute sans écrire un seul nombre.** `ld bc, __port_ram_audio |
__val_ram_audio`. Si je déplace la section, la valeur change avec elle ; si je
l'avais écrite en dur, elle serait devenue fausse sans un mot.

**Je remplis une ROM de 16 K avec les sections `"ro"` de six fichiers.** Je les
nomme toutes dans la même fenêtre ; le linker les concatène et me dit ce qui
reste : `2048 bytes unused at 0xE000 in rom_hi15`.

**Je découpe une banque de 16 K en deux blocs de 8 K.** `w3 [OFFSET 0x0000, SIZE
0x2000]` et `w3 [OFFSET 0x2000, SIZE 0x2000]`. Ce n'est pas une banque de 8 K :
les deux moitiés apparaissent ensemble ou pas du tout.

**Je dépasse la place disponible et on me le dit avec un chiffre.** Pas « ça ne
rentre pas » : *combien* ça dépasse, et de quelle banque.

**Je ne veux rien de tout cela.** `org &4000`, `run start`, aucune section, aucun
script, aucun `--target`. Les octets sont ceux d'aujourd'hui — c'est le §12.1, et
c'est un engagement.

**Auteur de profil.** J'écris ma machine dans un fichier de données et je la
passe en `-P`. Si cela demande de toucher au code du linker, le linker n'a pas
de modèle : il a des cas particuliers CPC, et je viens de le prouver.

**Mainteneur.** Je change le calcul de placement et une suite rougit à
l'endroit du calcul, sur des objets fabriqués à la main — pas trois étages plus
loin sur un octet de SNA.

## Décisions d'implémentation

### D1 — Le profil est un TEXTE EMBARQUÉ, lu par l'analyseur du script

Le §9 décide déjà que « les profils de cible sont compilés dans le binaire ». Il
ne dit pas sous quelle **forme**, et c'est la décision qui structure l'étage.

Le profil embarqué **est un fichier de profil**, un texte, lu par le même
analyseur que celui d'un `-P mien.prof`. Un modèle de données, un analyseur, un
chemin de code, **un seul jeu de valeurs**.

L'alternative — une `struct` C++ littérale, plus un analyseur ajouté plus tard —
a été écartée pour trois raisons, dans l'ordre de leur poids :

1. **Deux porteurs pour les mêmes valeurs.** Le §7 s'est pris cette faute dans
   la figure et en a tiré sa règle : *une valeur écrite deux fois est une valeur
   fausse une fois.* Un profil en `struct` **plus** un profil en texte est
   exactement la faute que le §7 combat.
2. **L'export exigerait un sérialiseur.** `--dump-profile` devrait écrire du
   texte depuis une `struct` : un second sérialiseur, qui peut diverger de
   l'analyseur — la faute précise que l'étape B7 a testée pour le `.fo` (écrire,
   relire, réécrire, comparer les textes). Avec D1, l'export est une **copie**,
   et le test est une comparaison d'octets.
3. **Un sérialiseur perdrait les citations, qui sont la valeur.** Dans un
   profil, les commentaires portent « sept sources concordantes », « non tranché
   par mesure », « mesuré sur 6128 le tant ». Le §12.3 fait de cette distinction
   *par valeur* le cœur de son argument. Un profil exporté sans sa provenance
   est un profil que personne ne peut auditer.

Et le coût qui semblait interdire ce dessin n'existe pas : **l'analyseur de
script est nécessaire de toute façon**, et les deux langages partagent leur
lexeur et leur grammaire à blocs — `CONFIG c { w1 { SECTION s } }` d'un côté,
`CONFIG SET ram { linear { w1 base1 } }` de l'autre. Le profil n'est pas un
second analyseur : c'est un second jeu de mots-clés sur le même.

**Conséquence sur le §10.** Sa ligne C1 dit « le linker qui place et calcule »
sans nommer de porteur. Elle n'est pas fausse, elle est muette ; cette décision
la complète et ne la corrige pas.

### D2 — Le langage de profil grandit avec ce qui le LIT

C1 analyse ce que le placement consomme, et rien de plus :

| notion (§13.1) | C1 | C2 |
|---|---|---|
| `WINDOW`, y compris plusieurs grilles superposées | oui | — |
| `BANK … SIZE …`, `ro` / `rw`, `VIDEO`, `CONTENDED` | oui | — |
| `CONFIG SET`, `CONFIG`, `OVER` | oui | — |
| `SELECT <axe> = OUT|POKE <port>, <valeur>` — **les nombres** | oui | — |
| `SELECT … STACK OUTSIDE`, fenêtre d'exécution interdite, `LOCKS`, séquences imposées, préconditions | — | oui |

**Un mot-clé non reconnu est REFUSÉ, jamais ignoré.** C'est la moitié
opérationnelle de cette décision : un profil écrit pour C2, passé à un fantams
C1 qui ignorerait ses contraintes, produirait un binaire faux **en silence** —
et une contrainte de commutation non vérifiée est précisément la faute
indétectable à l'exécution que le §13.3 décrit. Le refus nomme l'étage : *« STACK
OUTSIDE : reconnu à l'étage C2 »*.

`SHADOWS` et `ALWAYS` **ne sont pas des mots du langage** : le §13.1 les fait
calculer. Les écrire serait rouvrir la principale source d'erreur de saisie d'un
profil.

### D3 — `-P` est livré dès C1, parce que le profil embarqué n'a aucun privilège

Sous D1, `--target cpc6128` et `-P mien.prof` sont le même chemin de code.
Cacher le drapeau ne ferait rien gagner ; l'exposer **prouve** que le texte
embarqué n'est pas privilégié.

C'est aussi le seul test de généricité du modèle, et il est disponible pendant
l'étage où le vocabulaire se fixe — plutôt qu'après, quand il ne pourrait plus le
corriger. Le §13 pose que « décrire un ZX ou un MSX ne doit pas demander de
toucher au code du linker » ; D3 en fait une chose que quelqu'un peut **essayer**
au lieu d'une chose que la spec affirme.

Trois formes, et la troisième est une copie :

```
fantams game.asm -T game.ld --target cpc6128     # le texte embarqué
fantams game.asm -T game.ld -P zxnext.prof       # même analyseur, même code
fantams --dump-profile cpc6128 > cpc6128.prof    # copie, pas sérialisation
```

### D4 — La section est FUSIONNÉE PAR NOM à travers les objets

Aujourd'hui la table des bases est **par objet** : deux unités déclarant chacune
`SECTION code` reçoivent deux bases. Rien ne collisionne, mais le gain du §11 —
remplir une ROM avec les sections `"ro"` de dix fichiers — est hors d'atteinte.

Une section devient donc **une entité du linkage**, identifiée par son nom, dont
les fragments viennent de N objets et se suivent dans l'ordre où les objets sont
donnés.

Ce qui en découle, et qu'il faut écrire parce que ce sont des refus nouveaux :

- **le type est figé par la première déclaration**, à travers les objets comme
  il l'était déjà à l'intérieur d'un fichier (A0). Rouvrir en `"rw"` ce qu'un
  autre objet a déclaré `"ro"` désarmerait le contrôle en silence ;
- **deux plafonds différents pour un même nom sont refusés**, en nommant les deux
  sites. Retenir le plus petit serait défendable, et c'est la raison de refuser :
  personne ne peut deviner laquelle des deux lectures a été appliquée ;
- **l'ordre change** pour un programme multi-objet qui déclare plus d'une section
  relocalisable. Avant : `a1 b1 a2 b2`. Après : `a1 a2 b1 b2`. C'est le
  changement qui rend la concaténation possible, et un test l'épingle.

### D5 — L'`ORG` vient de la fenêtre de la grille à laquelle appartient la banque

Le §13.1 tranche le cas des grilles superposées, et c'est la règle à écrire une
fois : **il n'y a jamais d'arbitrage.** Une section placée dans un segment de
mapper de 8 K est basée par la fenêtre de mapper, pas par la page de slot qui la
contient, même quand les deux commencent à la même adresse.

Le placement se dit **dans une configuration**, jamais dans une fenêtre : c'est
la configuration qui sait quelle banque apparaît où, parce que sur la plupart des
machines les fenêtres ne se choisissent pas indépendamment.

Une section relocalisable que le script ne place pas ne devient pas une erreur :
elle retombe sur le placement dérivable du §9 — « à la suite, dans la région qui
correspond à son type ». Le cas simple ne paie ni en syntaxe, ni en fichier, ni
en argument de ligne de commande.

### D6 — `build` gagne deux ENTRÉES, `Image` n'en gagne aucune

```cpp
Image build(const std::vector<asmb::Object> &objects,
            const Script &script, const Profile &profile);
```

C'est la signature que `coutures-de-la-chaine.md` §2.2 prévoyait depuis le début.
Ce qui **ne bouge pas**, et c'est ce que l'étage B avait promis : `Image`,
`Block`, `Symbol`, `flatten`, et la façon dont chaque appelant lit le résultat.

Un script vide et un profil vide sont des valeurs licites, et elles donnent
**exactement** le placement de l'étage B. C'est ce qui rend le §12.1 vérifiable
par un test au lieu d'être une intention.

C1 et C2 ne sont pas deux appels : C2 ajoutera des diagnostics au même `Image`.
Les coutures `place()` / `check()` restent **internes**, privées à
l'implémentation et utilisables par ses tests. Sans quoi l'appelant devrait
savoir qu'après l'un il faut appeler l'autre, et C1 livré seul figerait cette
obligation partout.

### D7 — Les symboles de commutation sont un TRIPLET PAR AXE, jamais un octet

Le §12.3 donne la raison, et elle n'est pas CPC : sur ZX, `&7FFD` porte quatre
axes à la fois, et y sortir la seule valeur de l'axe de pagination écraserait les
trois autres en silence. Sur CPC au-delà de 512 K, ce n'est même plus la valeur
qui varie mais l'**adresse du port**.

| symbole | ce que c'est |
|---|---|
| `__port_<axe>_<config>` | l'adresse d'écriture — port d'un `OUT`, adresse d'un `POKE`. Indexée par la configuration, parce qu'elle peut en dépendre. |
| `__val_<axe>_<config>` | la valeur, **bornée aux bits de l'axe** |
| `__mask_<axe>` | les bits du port qui appartiennent à l'axe |
| `__off_<section>` | l'offset dans sa banque |
| `__romnum_<...>` | le numéro de ROM haute |

Mécaniquement, c'est **gratuit** : le linker porte déjà une table nom → adresse
pour les `PUBLIC`, et le refus « unresolved EXTERN symbol » couvre déjà le cas où
un nom n'existe pas. Les symboles de commutation s'y pré-remplissent.

**Les noms préfixés de `__` appartiennent au linker**, et sont réservés au même
titre que les mots de la machine (ADR 0015) : une source qui en définit un est
refusée.

Ce que le linker ne fournit **pas** : la copie de l'état. Un port en écriture
seule oblige la source à tenir en RAM la dernière valeur écrite ; c'est de la
RAM, donc du ressort de l'auteur. `__mask_<axe>` existe pour que cette copie se
mette à jour sans écraser les axes voisins ; le linker ne l'alloue pas.

### D8 — `BankOf` est écrit ici, et `bank()` est la fonction qui le produit

L'ADR 0027 réserve `BankOf` à C1, « qui seul connaîtra les banques ». C1 l'écrit,
et il lui faut une syntaxe : `bank(label)`, sur le modèle exact de `high()` et
`low()` de l'ADR 0027 — une **fonction explicite** plutôt qu'une règle devinée,
qui est la politique de l'ADR 0008.

Même dessin que `high()` / `low()` : légale sur une valeur relocalisable, elle
produit sa relocalisation ; le refus par défaut de tout le reste continue de
tenir.

### D9 — C1 NE COMPRESSE PAS, et refuse `COMPRESS` en nommant l'enveloppe

Le tableau du §10 range la compression en C1. **Cette spec l'en sort**, et c'est
la seule correction de périmètre qu'elle apporte.

La compression ne partage aucun calcul avec les fenêtres, les banques et les
configurations : il faut choisir un format, écrire ou embarquer un compresseur,
et le tester. C'est un second sujet dans un étage qui porte déjà deux langages
d'entrée — et la raison qui fait de C1 et C2 deux étages vaut ici mot pour mot :
les mêler garantirait qu'on livre l'un en promettant l'autre.

Le §8 laisse le repli, disponible depuis l'étage A : `SECTION blob, "ro", 0x2000`
déclare une **enveloppe**, tout ce qui suit a une adresse connue, et le linker
vérifie que le compressé rentre.

Ce que C1 écrit quand même, parce que c'est de la conception et non du code :
**l'ordre forcé** *placer → compresser → résoudre*, et la règle de sûreté qui va
avec — l'assembleur n'itère jamais, le linker peut itérer mais refuse bruyamment
la non-convergence. Un `COMPRESS` dans un script est refusé en nommant
l'enveloppe du §8.

### D10 — La CO-VISIBILITÉ est C2 : C1 place et constate, il ne vérifie pas les références

Le §12.2 présente le refus d'un appel de `main` vers `audio` comme un calcul de
C1 sur les configurations. C'est une lecture défendable, et ce n'est pas celle
qui est retenue.

- **C1 refuse les chevauchements de PLACEMENT** : deux sections qui se disputent
  des octets dans une banque, ou dans deux grilles superposées dont les fenêtres
  se recouvrent dans un même état. Décidable en regardant les seules décisions
  de C1.
- **C2 refuse les RÉFÉRENCES non co-visibles** : « un accès de `X` vers `Y` est
  licite s'il existe au moins un état où les banques de `X` et de `Y` sont
  simultanément visibles », décidé axe par axe, à coût constant.

La raison est que le §13.3 le dit lui-même : le piège du §12.2 n'est **qu'un cas**
de l'invariant de continuité, dont les trois pointeurs sont le PC, `SP` et le
vecteur d'interruption. Le séparer de ses deux frères donnerait un contrôle qui
attrape le tiers du problème en laissant croire qu'il le couvre — et le cas `SP`
est « le plus fréquent et le moins diagnostiqué ».

Et la frontière que C1/C2 existe pour tenir — *C1 produit un binaire, C2 refuse
un binaire faux* — se brouille dès que C1 se met à vérifier.

### D11 — Le plafond de 128 K est un refus du BUILDER, et il est déjà écrit

`link::kFlatBanks` vaut 8 : les 64 K de base plus l'extension du 6128, ce qu'un
dump plat sait porter. Au-delà, `fantams` refuse déjà et nomme la raison — *« a
flat dump stops at 128K … it needs the chunked v3 snapshot »*.

C1 ne touche pas à cette limite et n'en invente pas d'autre : il **passe par ce
refus**. Une conséquence à écrire, parce qu'elle borne le livrable : le profil CPC
de C1 est un 6128 + RAM128, et une machine à 512 K se décrit dans le langage sans
pouvoir s'exporter. Le SNA v3 à chunks, le `.dsk` multi-fichiers et le `.cro`
sont du travail de **builder**, que le tableau du §10 ne numérote pas.

### D12 — Le cas simple ne paie rien, et c'est un test

Le §12.1 est un engagement de compatibilité. Sans script, sans `--target` et sans
`-P` :

- une source avec `org` et `run` produit les **mêmes octets** qu'aujourd'hui ;
- `examples/demo.asm`, `separate_a.asm` + `separate_b.asm` et `separate_mono.asm`
  produisent les mêmes binaires qu'avant l'étage ;
- aucun diagnostic ne change d'un caractère.

Ce n'est pas un vœu de fin d'étage : c'est un contrôle tenu à **chaque étape**,
au même titre que les dix suites vertes.

### D13 — L'ordre des étapes est forcé par deux invariants

1. **Le dépôt compile et les dix suites sont vertes à chaque étape** — les neuf
   d'origine plus `accept_separate`, comme pendant tout l'étage B.
2. **D12 tient à chaque étape.**

L'appel de conception de l'étage est que **deux étapes se testent seules, avant
qu'aucun placement ne change** : l'analyseur de script (texte → `Script`) et
l'analyseur de profil (texte → `Profile`) sont des fonctions pures qu'aucun
octet ne traverse. Et une troisième est indépendante des deux : la fusion par
nom (D4) est un changement de placement pur, testable au point le plus haut avec
des objets fabriqués à la main, sans un mot de profil.

Le lexeur partagé **n'est pas un préfacteur** : l'écrire avant d'avoir deux
consommateurs serait la couture hypothétique que
`coutures-de-la-chaine.md` §4 interdit. Il s'extrait à l'étape du profil, qui est
le deuxième consommateur.

### D14 — Où vont les ADR

Un seul ADR de clôture, et il porte **D1** — le profil est un texte embarqué, un
porteur, un analyseur, un export qui est une copie. C'est la décision que la
prochaine personne à ouvrir le code ne pourra pas reconstituer.

**L'ADR 0005 est relu à la clôture.** Le §12.2 dit que C1 le remplace : la
question qu'il tranchait — comment la source nomme un emplacement de rangement —
ne se pose plus, puisque la source ne nomme plus d'emplacement du tout. Il
continue de décrire `org b<n>:`, qui reste licite ; son statut est à écrire, pas
à deviner.

Les décisions prises en cours de route s'écrivent **dans l'étape qui les
provoque**, comme à l'étage B : une décision écrite à la fin est une décision que
personne n'a lue au moment où elle comptait.

## Décisions de test

**Ce qu'est un bon test ici** : il porte sur un comportement observable à une
couture, et il ne demande pas d'écrire un source Z80 pour provoquer une faute de
placement.

**Quatre coutures de test, dont deux existent déjà.**

| couture | entrée → sortie | ce qu'on y vérifie |
|---|---|---|
| `parseScript` | texte → `Script` | la syntaxe, les diagnostics, et qu'un mot inconnu est refusé |
| `parseProfile` | texte → `Profile` | idem, plus le refus des mots de C2 en nommant l'étage |
| `link::build` | objets **fabriqués à la main** + script + profil → `Image` | tout le calcul de placement. C'est le gain décisif : un chevauchement inter-sections se teste avec deux structures de dix lignes. |
| `assemble` | source → `Object` | `bank()` produit sa relocalisation ; rien d'autre ne change |

**Ce qui se teste par comparaison d'octets** : `--dump-profile cpc6128` contre le
texte embarqué. C'est le seul contrôle qui attrape la divergence que D1 existe
pour rendre impossible.

**Le critère de fin d'étage** est un **exemple d'acceptation** : l'exemple du
§12.2, **sans sa section compressée** (D9), assemblé, linké et exporté en SNA
128 K, avec quatre contrôles tenus par le script de test —

- les octets de `main` sont dans la banque 1, ceux de `sysbank` dans la banque 2,
  ceux d'`audio` dans la banque 5 ;
- `audio_init` vaut `&4000 + offset`, sans qu'un `org` l'ait dit ;
- `__val_ram_audio` vaut `&C5`, `__val_ram_linear` vaut `&C0` ;
- déplacer `audio` d'`ext1` vers `ext2` **dans le script seul** change la banque
  de rangement et la valeur de commutation, et **pas une adresse logique**.

Le dernier est celui qui compte : il est la preuve que le renversement du §12.2 a
eu lieu. Le script est inscrit dans les **deux** listes de tests, `Makefile` et
`CMakeLists.txt`.

## Hors périmètre

- **La compression** (D9) — mécanisme et ordre écrits, algorithme non livré.
- **Tout C2** : la continuité et ses trois pointeurs, les sections miroir,
  l'accord structurel au même offset, l'équivalence consignée, `CLOBBERS`,
  `INIT_FROM`, la co-visibilité des références (D10), et les contraintes de
  `SELECT` (D2).
- **Le builder** : le SNA v3 à chunks, `.dsk`, `.cdt`, `.cpr`, `.cro`, et le
  nettoyage de la signature plate de `sna::build` — dette réelle, nommée par
  l'ADR 0027, et qui ferait de C1 un étage à deux sujets.
- **L'étage D** : la lecture des `.rel` de SDCC.
- **Le refus de placement d'une section écran dans une banque sans `VIDEO`** : le
  profil porte l'attribut dès C1, mais rien ne marque une section comme étant de
  l'écran. C'est un mot de vocabulaire de section, et il n'y a pas de raison de
  l'inventer avant d'en avoir besoin.
- **Un deuxième profil livré.** Le §13.2 pose que le danger est de « payer la
  conception de trois machines pour n'en livrer aucune ». Un ZX ou un MSX écrit
  **en test**, pour exercer le vocabulaire, est un bon investissement ; livré,
  non.

## Notes

**Ce document corrige deux textes existants.**

Le tableau du §10, qui range la compression en C1 : elle en sort (D9). La ligne
est à amender dans `spec-chaine-outils.md`, à l'étape qui l'établit.

L'ADR 0005, dont le §12.2 annonce que C1 le remplace : son statut est à écrire à
la clôture (D14).

**Il en complète un troisième.** Le §9 décide que les profils sont compilés dans
le binaire sans dire sous quelle forme ; D1 le dit.

**Ce que l'étage C1 ne change pas, et c'est délibéré** : le préprocesseur, qui
tourne avant qu'aucune adresse existe et n'a rien à savoir des sections ni des
banques ; l'assembleur, hors la fonction `bank()` de D8 ; et le format `.fo`,
dont C1 ne touche aucun bloc — un objet reste ce qu'une unité de compilation
contient, et le placement reste ce qu'il ne contient pas.
