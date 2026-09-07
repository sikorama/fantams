# Spécification : assembleur, linker, builder

> **Rien de ce document n'est écrit.** Il fixe la cible : ce qu'un assembleur
> Z80A doit faire, ce qu'il doit refuser de faire, et par quelles informations il
> passe la main aux deux outils qui le suivent. Le code actuel n'a ni section, ni
> fichier objet, ni linker, ni builder ; le §10 dit ce qu'il en coûterait, et par
> quels étages y aller.

## 1. Le périmètre d'un assembleur

Le rôle strict d'un assembleur est d'associer des mnémoniques et des symboles à
du code machine. Ses responsabilités, et rien d'autre :

- **Traduction du jeu d'instructions** — mnémoniques Z80A vers opcodes et
  opérandes.
- **Table des symboles** — labels, équivalences, adresses logiques et physiques,
  expressions.
- **Structuration des données** — octets, mots, chaînes, réservations.
- **Contrôle du flux d'assemblage** — assemblage conditionnel, macros,
  répétitions.
- **Portée mémoire courante** — l'emplacement d'assemblage, et le refus des
  chevauchements.
- **Métriques du code produit** — taille, et durée d'exécution (§4.3).

Ce périmètre a une conséquence directe : **l'assembleur ne connaît ni l'Amstrad
CPC, ni aucun format de fichier hôte.** Il ne traite que de la mémoire adressable
du Z80. Le même assembleur sert un Spectrum, un MSX ou une Master System sans
porter une ligne de code spécifique au CPC.

## 2. Ce qui n'appartient pas à l'assembleur

Le contre-exemple est rasm, qui n'est pas un assembleur mais une chaîne
complète — assembleur, linker, générateur d'images de support et moteur
d'encodage — pilotée depuis le source. Les directives à écarter, par famille :

**Conteneurs et métadonnées de machine hôte.** `BUILDCPR`, `BUILDTAPE`,
`BUILDSNA`, `BUILDZX`, `BUILDROM`. Elles forcent l'assembleur à connaître la
structure interne de dizaines de formats de fichiers propres à des machines
hôtes. Le conteneur final décrit une livraison, pas un programme.

**Configuration matérielle et état d'émulateur.** `SETCPC`, `SETCRTC`, `SNASET`.
Injecter des valeurs de registres de puces auxiliaires — CRTC, Gate Array, PSG —
ou l'état d'un émulateur depuis le source détourne l'assembleur en
configurateur de machine virtuelle.

**Pagination matérielle.** `{BANK}`, `{PAGE}`, `{PAGESET}`. Commodes, mais elles
exigent de l'assembleur la logique du Gate Array du CPC — les ports `&7FC5`,
`&7FC2`. C'est une carte mémoire de machine, pas un espace d'adressage Z80.

**Point d'entrée de sortie.** `RUN`, quand il renseigne l'adresse d'exécution et
le pointeur de pile d'un snapshot ou d'un en-tête AMSDOS. Cela relève du format
de sortie.

Restent légitimes, parce qu'elles ne touchent qu'à l'organisation de l'espace
d'adressage et à l'émission des octets : `ORG` logique et physique, `ALIGN`,
`LIMIT` / `PROTECT`, `NOCODE` / `CODE`.

### Le cas des directives de transformation de données

Le chiffrement à l'assemblage n'est pas illégitime : son résultat est une donnée
assemblée, au même titre que le résultat d'un calcul de macro. Le grief porte sur
le design, non sur la place.

`SUMMEM`, `XORMEM` et `CIPHERMEM` font peu ou prou la même chose en trois
directives, et le paramètre « clef » de `CIPHERMEM` n'est nulle part défini. Une
directive d'altération d'un flux d'octets doit être **générique** :

```
TRANSFORM debut, fin, expression
```

où `expression` est un XOR, une addition, ou une fonction de l'adresse courante.
Une directive, un concept.

Ce qui rend cette directive légitime dans l'assembleur n'est pas qu'elle soit
pratique, c'est qu'elle soit **de taille constante** : aucune adresse ne bouge.
D'où la ligne générale, qui vaut aussi pour la compression (§8) :

> Une transformation qui préserve la taille appartient à l'assembleur ; une
> transformation qui change la taille appartient au linker.

## 3. Les trois maillons

```
[ Source Z80 ]  ──▶  ( 1. ASSEMBLEUR )
                            │
              [ Fichiers objets : octets + sections + symboles ]
                            │
[ Script de linkage ]  ──▶  ( 2. LINKER / MAPPER )
                            │
                 [ Image binaire structurée ]
                            │
[ Profil de cible ]   ──▶   ( 3. BUILDER / PACKAGER )
                            │
                [ DSK, SNA, CRO, CPR, ROM ]
```

### 3.1 L'assembleur

Périmètre du §1. Il produit un **fichier objet** : les octets, la table des
sections avec leurs attributs, la table des symboles et des relocalisations
(§4).

### 3.2 Le linker / mapper

C'est **ici, et seulement ici**, que vit la connaissance de la carte mémoire de
la machine cible. Il reçoit un ou plusieurs fichiers objets et un script de
linkage, et :

- affecte les sections aux espaces physiques du CPC ;
- calcule les adresses définitives et résout les relocalisations ;
- vérifie l'absence de chevauchement entre banques ;
- rend compte des symboles inter-banques, et fournit à chaque label la banque qui
  le porte ;
- **fournit au source les valeurs de commutation** — la configuration PAL, le
  numéro de ROM, le contenu de `RMR` — sous forme de symboles, pour qu'aucune de
  ces valeurs ne soit écrite en dur (§12.3) ;
- **vérifie la continuité à travers une commutation** : une section placée dans
  une banque étendue occupe, une fois commutée, la place d'une section de la RAM
  de base — ce qui concerne le PC, la pile et le vecteur d'interruption (§13.3).

Les espaces physiques d'un CPC qu'il doit savoir décrire :

- **RAM de base** — 64 K, quatre pages de 16 K.
- **RAM étendue** — jusqu'à 4 Mo, par tranches commutables de 16 K.
- **ROM basse** — `&0000`–`&3FFF`.
- **ROM haute** — `&C000`–`&FFFF` sur CPC standard.
- **ROMs du CPC Plus** — également mappables sur `&4000`–`&7FFF` et
  `&8000`–`&BFFF`.
- **ROMs de taille non standard** — une ROM Multiface occupe `&0000`–`&1FFF`,
  soit 8 K.

### 3.3 Le builder / packager

Il prend les images mémoire du linker et les intègre dans les formats de
distribution ou de test :

- **supports** — `.DSK` (AMSDOS), `.CDT` (cassette), `.CPR` (cartouche Plus) ;
- **tests** — `.SNA`, avec le positionnement des registres Z80, CRTC et Gate
  Array pour un démarrage immédiat en émulateur ;
- **ROMs** — export standard, ou format `.CRO` (Longshot / Logon System) qui
  porte les métadonnées de ROMs et initialise proprement l'environnement
  d'émulation.

Le SNA cesse alors d'être le véhicule universel. Aujourd'hui, produire une ROM
consiste à la loger dans un SNA — galvaudage du format : une ROM n'est pas un
état machine, elle appartient à un **contexte d'exécution** qui initialise
l'émulation. C'est exactement ce que `.CRO` décrit.

Le conteneur final n'est qu'un pilote de sortie : le même binaire part en `.CRO`
ou en `.SNA` sans modifier une ligne du source.

## 4. L'interface assembleur → linker

### 4.1 Les sections

Une **section** est une unité logique d'assemblage dont les adresses sont
relatives à `0x0000`, donc relocalisables. Elle est un espace de travail pour
l'assembleur ; c'est le linker qui décide de son emplacement physique.

```
SECTION nom, "type" [, taille_max]
```

Les trois types, qui sont la seule sémantique matérielle que l'assembleur
connaisse :

| type       | contenu                                | émet des octets |
|------------|----------------------------------------|-----------------|
| `"ro"`     | code exécutable et constantes          | oui             |
| `"rw"`     | données initialisées modifiables       | oui             |
| `"uninit"` | emplacement réservé, non initialisé    | non             |

**La taille maximale est déclarative et vérifiée à l'assemblage.** On peut
vouloir limiter une section audio à `0x2000` parce qu'on destine le reste d'une
ROM de `0x4000` à autre chose :

```
SECTION Audio_Code, "ro", 0x2000
```

Le dépassement est une erreur immédiate, sans attendre le linkage :

```
Error: Section 'Audio_Code' exceeds maximum declared size (0x2140 > 0x2000 bytes).
```

`ASSERT_SIZE max` vérifie de la même façon une sous-zone à l'intérieur d'une
section — une table de saut, par exemple.

Le linker garde la vérification qu'il est seul à pouvoir faire : que la **somme**
des sections affectées à un composant matériel tient dans sa capacité.

### 4.2 Détection statique des écritures en ROM

Connaissant le type de la section qui porte chaque symbole, l'assembleur peut
refuser une écriture vers une section `"ro"` :

```
SECTION tables_data, "ro"
mon_tableau:  db 1, 2, 3, 4

SECTION execution, "ro"
        ld a, 5
        ld (mon_tableau), a
        ; Error: "ld (nn), a" writes into read-only section 'tables_data'
```

C'est une vérification statique, adossée au type de section associé au symbole.

### 4.3 Mesure du temps d'exécution

L'assembleur connaît le coût en cycles de chaque instruction ; c'est donc le seul
endroit où la mesure soit exacte, et son absence est une lacune.

```
CYCLES_BETWEEN(label1, label2)
```

La valeur rendue est utilisable dans une expression, donc paramétrable : une
macro peut générer un délai à partir d'une durée mesurée, comme `ALIGN` génère un
remplissage jusqu'à une frontière.

### 4.4 Portée des symboles

- `PUBLIC label` — le symbole est référençable depuis un autre objet.
- `EXTERN label` — le symbole n'est pas défini ici ; l'assembleur laisse une
  entrée de relocalisation au linker.

### 4.5 Adresse d'assemblage et adresse d'exécution

Sur CPC, il est courant de stocker du code en ROM ou en RAM étendue et de le
copier en RAM centrale avant de l'exécuter. Les deux adresses diffèrent :

- `PHASE adresse` — les labels sont calculés depuis l'adresse de destination,
  tandis que les octets continuent d'être émis dans la section courante ;
- `DEPHASE` — annule le décalage.

### 4.6 Le fichier objet

Trois blocs :

1. **Le flux d'octets** — code et données bruts, par section.
2. **La table des sections** — nom, type, taille, taille maximale déclarée.
3. **La table des symboles et des relocalisations** — pour chaque symbole : sa
   section, son offset, sa portée (`PUBLIC`, `EXTERN`, local).

## 5. Contraintes de placement : `BOUNDARY`

`CONFINE valeur` est une rustine : elle répond à un besoin sans le nommer, son
argument est une quantité d'octets qui doit rester inférieure à 256, et elle
réalise en fait un `ALIGN`. C'est donc `ALIGN` qui devrait porter cette logique —
mais un `ALIGN frontiere, remplissage, padding_max` reste illisible : il
demande à l'auteur de calculer lui-même la taille de sa table et de la passer en
chiffre magique.

Le besoin réel n'est pas un alignement, c'est un **contrat d'allocation** : cette
structure de N octets ne doit pas croiser une frontière de M octets — 256,
typiquement, pour que le registre H ne change pas en la parcourant.

La forme lisible est un bloc auto-mesuré :

```
        BOUNDARY 256
my_table:
        dw label_1
        dw label_2
        db #FF
        END_BOUNDARY
```

L'assembleur mesure le bloc lui-même (ici 5 octets), regarde l'adresse courante,
et applique une règle unique :

> Émettre à la suite si le bloc tient entièrement dans la page courante ; sinon
> sauter au début de la page suivante.

À `&2FFE`, les 5 octets ne tiennent pas dans les deux octets restants : le bloc
part en `&3000`. À `&2F00`, il est émis sur place, sans aucun saut.

Si le bloc est plus grand que la frontière, la condition ne peut jamais être
satisfaite, et c'est une erreur d'assemblage :

```
Error: Block 'my_table' (300 bytes) exceeds the boundary limit (256 bytes).
```

Aucun paramètre obscur, aucune taille calculée à la main, aucun comportement à
mémoriser.

## 6. Le script de linkage

Toute la configuration d'architecture qui truffait le source — `BUILDSNA`,
`BANK 5`, `SETCRTC` — est extraite dans le linker.

Le vocabulaire employé ici (`PAL_PAGE`, `RMR_BIT`, `PORT`) est **l'instance
CPC** d'un modèle plus général : le §13 en donne la forme génerique, celle qui
doit aussi décrire un ZX 128 ou un MSX.

```
// CPC 6128 + extension RAM 128 K + ROMs
MEMORY_MAP {

    // RAM principale, 64 K en quatre pages de 16 K
    REGION RAM_BASE [0x0000..0xFFFF] {
        PAGE 0: [0x0000..0x3FFF]
        PAGE 1: [0x4000..0x7FFF]
        PAGE 2: [0x8000..0xBFFF]
        PAGE 3: [0xC000..0xFFFF]
    }

    // RAM étendue : page de 64 K n° 1, pilotée par le PAL (11pppccc)
    REGION RAM_EXP1 [PAL_PAGE 1] {
        PAGE_EXT 0: [0x4000..0x7FFF]    // config %100
        PAGE_EXT 1: [0x4000..0x7FFF]    // config %101
        PAGE_EXT 2: [0x4000..0x7FFF]    // config %110
        PAGE_EXT 3: [0x4000..0x7FFF]    // config %111
    }

    REGION ROM_LOWER [ADDRESS 0x0000, RMR_BIT 2]

    // ROM de 16 K découpée en deux blocs de 8 K
    REGION ROM_UPPER [ADDRESS 0xC000, RMR_BIT 3, PORT 0xDF00, SIZE 0x4000] {
        ROM_SLOT 15 {
            BLOCK Audio_Block [OFFSET 0x0000, SIZE 0x2000] {
                SECTION Audio_Code
            }
            BLOCK Data_Block  [OFFSET 0x2000, SIZE 0x2000] {
                SECTION Graphics_Data
                SECTION Menu_Text
            }
        }
    }
}

OUTPUT_FORMAT {
    TARGET      = "SNA_V2"
    ENTRY_POINT = 0x8000
    STACK       = 0x3FFF
    // ou, pour une ROM :
    // TARGET = "CRO"  ;  CRO_ROM_NUMBER = 15
}
```

Le linker peut alors signaler `2048 bytes unused in Audio_Block`, ou y loger une
section marquée comme déplaçable.

## 7. La carte mémoire du CPC, pour le linker

C'est la connaissance que le linker doit porter, et que l'assembleur doit
ignorer.

### ROMs, par le Gate Array

Le registre `RMR` du Gate Array détermine quelles ROM sont visibles dans l'espace
adressable :

```
RMR = 100vRrmm
        │││└┴── mm : mode écran
        ││└──── r  : 1 = ROM basse activée
        │└───── R  : 1 = ROM haute activée
        └────── v  : 1 = remise à 0 du compteur vsync (toujours 0 ici)
```

Sur CPC standard, le **numéro** de ROM haute est choisi par le port `&DF00`. Sur
CPC Plus, `RMR2` étend le mécanisme et permet de mapper des ROMs sur
`&4000`–`&7FFF` et `&8000`–`&BFFF`.

### RAM étendue, par le PAL

La RAM est pilotée par un PAL, à la même adresse d'entrée-sortie que le Gate
Array :

```
11pppccc
  │││└┴┴── ccc : configuration
  └┴┴───── ppp : numéro de page de 64 K additionnelle, 0 à 7
```

| `ccc` | effet dans l'espace adressable du Z80A                                        |
|-------|-------------------------------------------------------------------------------|
| `000` | aucune RAM étendue connectée                                                  |
| `001` | 4ᵉ page de la RAM étendue (page `ppp`) en `&C000`                             |
| `010` | les 64 K entiers de la page étendue basculent dans l'espace adressable        |
| `011` | les 16 K habituellement en `&C000` passent en `&4000`, et la 4ᵉ page étendue en `&C000` |
| `100` | 1ʳᵉ page du bloc de 64 K additionnel en `&4000`–`&7FFF`                       |
| `101` | idem avec la 2ᵉ page                                                          |
| `110` | idem avec la 3ᵉ page                                                          |
| `111` | idem avec la 4ᵉ page                                                          |

## 8. Données compressées

Un bloc compressé a une taille qui dépend de son contenu. Cela casse l'invariant
qui rend l'assemblage en deux passes possible — la taille d'une instruction
dépend du **type** de ses opérandes, jamais de leur valeur — et c'est par là
qu'un assembleur se met à itérer vers un point fixe.

La règle du §2 tranche le placement : la compression change la taille, elle
appartient donc au linker, déclarée là où le placement est déclaré, jamais comme
une directive du source. La crainte se dissout ensuite en trois cas distincts.

**La taille est un symbole, pas une constante d'assemblage.** C'est le cas
courant : un blob compressé que le code dépacke à l'exécution. Le code a besoin
de `blob_start` et de `blob_size`, pas de leur valeur *pendant* l'assemblage.
`ld bc, blob_size` émet une relocalisation ; le linker compresse, connaît la
taille, et patche. Aucun point fixe, aucune itération. C'est à cela que sert la
relocalisation — et c'est le meilleur argument pour l'étage B de la migration
(§10) : sans objet relocalisable, il faut effectivement connaître la taille à
l'assemblage ; avec, la question ne se pose plus.

**Le budget, quand il n'y a pas de relocalisation.** Une taille maximale déclare
une **enveloppe**, pas une taille :

```
SECTION blob, "ro", 0x2000
```

Tout ce qui suit a une adresse connue, le linker vérifie que le compressé rentre
et signale le mou. La taille réelle n'a plus d'effet sur aucune adresse. C'est le
repli utilisable dès l'étage A.

**Le cas circulaire est une affaire de politique de placement.** Quand l'adresse
de quelque chose dépend de la taille compressée d'autre chose, la réponse n'est
pas d'itérer : c'est de **placer les sections compressées en dernier dans leur
région**. Rien ne dépend alors de leur taille. Une politique de placement est
précisément ce que seul un linker sait exprimer — le découpage résout le problème
au lieu de le créer.

Et la règle de sûreté qui va avec : **l'assembleur n'itère jamais ; le linker
peut itérer, mais il refuse bruyamment la non-convergence.** C'est ce que rasm ne
fait pas avec ses segments compressés, et c'est pourquoi il peut produire du code
*apparemment* correct.

## 9. Un binaire, trois modes

Le cœur est une fonction pure, sans état ni entrées-sorties, exposée à chaque
hôte par un adaptateur fin. **Le nombre d'exécutables est donc une affaire
d'adaptateur, pas d'architecture** : la séparation assembleur / linker est une
séparation de *modules* — un `link.cpp` à côté d'`asm.cpp`, tous deux appelables
sans système de fichiers.

Ce n'est pas une indifférence, c'est un argument contre deux exécutables : deux
binaires forceraient les fichiers objets à transiter par un système de fichiers
virtuel dans l'hôte navigateur. C'est exactement le CLI émulé dont le projet veut
se défaire, et qui a déjà produit ses seules divergences d'octets.

Donc un binaire, qui est un **driver**, sur le modèle de `cc` :

```bash
fantams game.asm -o game.sna                # assemble + link + package,
                                            #   sans qu'aucun .o touche le disque
fantams -c audio.asm -o audio.o             # s'arrête à l'objet
fantams audio.o main.o -T cpc.ld -o d.dsk   # link + package
```

Le mode se déduit de la nature des entrées. La règle de salubrité est que **le
chemin en une commande passe par la même fonction de linkage, sur les mêmes
structures, que le chemin en trois** : sinon la frontière pourrit, faute d'être
exercée par le cas courant.

### Le cas simple ne paie rien

C'est la contrepartie non négociable du découpage. Elle s'obtient par trois
défauts, non par un compromis d'architecture.

**Les profils de cible sont compilés dans le binaire.** `--target cpc6128` nomme
une carte mémoire intégrée ; `-T ma_carte.ld` la remplace. Personne n'écrit un
script de linkage pour faire un programme 6128 — c'est ce que fait `gcc` avec son
script par défaut que personne n'a jamais vu. Le fichier d'architecture n'existe
que pour ce que le profil ne sait pas dire.

**Pas de `SECTION` : une section implicite.** Une source avec `org &4000` et
aucune section est une section unique, absolue, placée exactement où son `org` le
dit. Aucune relocalisation, aucun objet, octets identiques à aujourd'hui.

**Le placement reste dérivable.** Une section que le profil ne place pas
explicitement va, à la suite, dans la région qui correspond à son type. Seule une
source qui veut vraiment du banking ou des slots ROM écrit une carte.

> Le cas simple ne paie ni en syntaxe, ni en fichier, ni en argument de ligne de
> commande.

Le coût réel du découpage n'est donc pas un coût d'usage mais un coût de
**documentation** : deux concepts de placement au lieu d'un. Il se paie en une
phrase — *un `org` place, une `SECTION` délègue le placement* — et si cette
phrase ne suffit pas, c'est le découpage qui est mauvais.

### Interopérabilité avec SDCC

Le gain est réel, mais dans un sens précis : c'est **le linker de fantams qui
doit lire les objets de SDCC**, non l'inverse. SDCC/Z80 produit des `.rel` au
format ASxxxx — des *areas* avec attributs, une table de symboles, des
enregistrements de relocalisation — et les lie avec `sdld`, dont la gestion du
banking CPC est le point faible. Un linker qui connaît `RMR`, `&DF00` et
`11pppccc` est exactement ce qui manque à cette chaîne. Émettre du `.rel` pour se
faire lier par `sdld` nous soumettrait au contraire à ses limites.

Conséquence de conception : le modèle d'objet doit rester *alignable* sur le
modèle ASxxxx — une section ≈ une *area*, et les types `"ro"` / `"rw"` /
`"uninit"` se projettent sur les conventions `_CODE` / `_DATA` / `_BSS` de SDCC.
Ne pas inventer un modèle plus riche que ce que `.rel` sait exprimer, sous peine
de ne pouvoir traduire que dans un sens.

> **À vérifier avant de s'engager.** Les détails du format `.rel` et des
> conventions d'appel de SDCC énoncés ici sont restitués de mémoire, non
> vérifiés sur la documentation ASxxxx ni sur le manuel SDCC.

## 10. Migration du code actuel

### Ce qui est déjà à mi-chemin

Le modèle mémoire. `asm.cpp` porte une collection d'espaces de 16 K indexée par
un entier :

```cpp
struct Space { std::vector<uint8_t> bytes; std::vector<uint16_t> prov; };
std::map<int, Space> spaces_;
```

Un `Space` est déjà « un bloc d'octets avec sa propre coverage, alloué à la
première écriture, indexé par un entier dont on ne préjuge pas le sens ». Une
section est cette généralisation : la clé cesse d'être un numéro de banque pour
devenir une identité de section, et le `Space` porte en plus son nom, son type,
sa taille maximale et son placement. `emit()` change de trois lignes — au lieu de
dériver banque et offset de l'adresse, l'octet va dans la section courante à
l'offset courant.

La propriété qui rend le reste possible est là, et gratuite : **la taille d'une
instruction dépend du type de ses opérandes, pas de leur valeur.** C'est
exactement l'invariant qui permet la compilation séparée sans passe de
relaxation.

La table des symboles exportable est un embryon de fichier objet : nom, nature,
adresse logique, banque et adresse de rangement, fichier et ligne d'origine. Le
format objet peut en être l'extension plutôt qu'une invention.

### Ce qui coûte

**Une seule chose : la relocalisation.** Aujourd'hui un symbole est un nombre —
le résolveur d'`expr` rend un `double`, et la table des symboles de l'assembleur
est un `map<string, double>`. Il faut que la valeur devienne `(section, offset)`
et que l'évaluateur sache qu'une expression est **affine** en bases de
sections : `label2 - label1` est absolu si les deux labels partagent leur
section, `label` seul est relocalisable, `label * 2` est illégal en contexte
relocalisable. C'est une reprise d'`expr.cpp` et de la table des symboles, et
c'est le seul morceau qui ne se découpe pas en petits pas.

Un saut relatif inter-sections ne peut pas être résolu par l'assembleur : il
devient une relocalisation dont le linker vérifie la portée.

Ne sont pas concernés : `z80.cpp`, `keywords.cpp`, `parser.cpp`, `beautify.cpp`,
et `pp.cpp` — le préprocesseur n'a rien à savoir des sections.

### Les trois étages

| étage | ce qu'on gagne | relocalisation |
|-------|----------------|----------------|
| **A.** `SECTION` interne, placement toujours absolu (`org` à l'intérieur) | plafond de taille, détection d'écriture en `"ro"`, section dans la table des symboles | non |
| **B.** fichier objet et expressions relocalisables | compilation séparée, `PUBLIC` / `EXTERN`, tailles résolues au linkage (§8) | oui — le gros morceau |
| **C.** le linker : régions, banques, ROMs, chevauchements inter-sections | le maillon 2 du §3.2 | — |

Les étages A et B sont indépendants. **L'étage A est un investissement autonome,
pas une demi-mesure** : le plafond de section et le refus d'écriture en ROM sont
utiles seuls, se testent seuls, et ne se paient pas dans `expr.cpp`.

## 11. Ce que gagne ce découpage

**Le source est agnostique.** Aucun couplage au CPC dans le `.asm` : il est
réutilisable, et l'assembleur reste rapide, déterministe et focalisé sur la
traduction d'instructions.

**Les fautes sortent au plus tôt.** Écriture en ROM, dépassement de taille de
section, bloc qui ne peut pas tenir dans sa frontière : l'assembleur refuse sans
qu'on ait lancé le linker.

**Le matériel est explicite.** `RMR`, `RMR2`, `11pppccc`, `&DF00` sont écrits
dans le script de linkage, non masqués derrière une macro opaque comme
`{PAGESET}`.

**Le linker optimise ce que l'assembleur ne voit pas.** Concaténer les sections
`"ro"` de dix fichiers pour remplir au plus juste une ROM de 16 K est un enfer à
la main, avec des `ORG` et des `BANK` ; c'est un travail de linker.

**Les formats deviennent interchangeables.** Le conteneur est un pilote de
sortie : `.CRO` ou `.SNA` depuis le même binaire, sans toucher au source.

## 12. Deux exemples de bout en bout

### 12.1 Le cas simple : rien ne change

Une source, un `org`, aucune section, aucun script :

```
        org  &4000
        run  &4000
start:
        ld   hl, message
        call print
        jp   start
message:
        db   "hello", 0
```

```bash
fantams game.asm -o game.sna
```

Une invocation, un fichier en sortie, deux passes d'assemblage. Aucun objet
intermédiaire, aucun linkage à décrire : la source est **une section implicite,
absolue, placée là où son `org` le dit**. C'est le comportement actuel, mot pour
mot, et le découpage ne doit rien y changer.

### 12.2 Le cas complexe

L'énoncé : du code en RAM centrale à `&4000`, un player audio dans une banque de
la RAM étendue, un bloc de données compressé ailleurs, dépacké en `&C000`.

#### Le plan mémoire, et le piège qu'il contient

| section | type | emplacement | fenêtre Z80 |
|---------|------|-------------|-------------|
| `main` | `"ro"` | RAM de base, page 1 | `&4000`-`&7FFF` |
| `sysbank` | `"ro"` | RAM de base, page 2 | `&8000`-`&BFFF` |
| `audio` | `"ro"` | extension, banque 5 | `&4000`-`&7FFF`, config `%101` |
| `music_lz` | `"ro"`, compressée | extension, banque 4 | `&4000`-`&7FFF`, config `%100` |
| `unpacked` | `"uninit"` | RAM de base, page 3 | `&C000`-`&FFFF` |

Le piège est dans la dernière colonne : les configurations `%100` à `%111`
paginent la banque étendue **à la place** de la page 1 de la RAM de base. Or
`main` est justement là. Commuter `audio` fait donc disparaître `main` sous ses
propres pieds.

C'est pour cela que `sysbank` existe : la commutation, l'appel au player et le
dépacking vivent en page 2, qu'aucune de ces configurations ne recouvre. **Cette
contrainte, l'assembleur ne peut pas la connaître — elle est dans la carte
mémoire de la machine. Le linker, si.** Il refuse un appel depuis `main` vers
`audio`, en nommant les deux sections et la fenêtre qu'elles partagent, plutôt
que de laisser produire un programme qui se sabote à la première commutation.
Le §13.3 donne la règle générale dont ce refus n'est qu'un cas, et la seconde
façon de le satisfaire quand aucune fenêtre ne reste résidente.

#### Les sources : une seule suffit

Rien n'oblige à découper. Un seul fichier porte les cinq sections :

```
;--- game.asm -------------------------------------------------------------
        SECTION main, "ro"
        run    start
start:
        call   sysbank_unpack
        call   sysbank_audio_init
loop:   call   sysbank_audio_play
        jr     loop

;--- résident : jamais recouvert par une commutation ----------------------
        SECTION sysbank, "ro"
sysbank_audio_init:
        ld     bc, &7F00 + __cfg_audio
        out    (c), c
        call   audio_init          ; vaut &4000 + offset : le linker le sait
        ld     bc, &7F00 + __cfg_none
        out    (c), c
        ret

sysbank_unpack:
        ld     bc, &7F00 + __cfg_music_lz
        out    (c), c
        ld     hl, music_lz        ; &4000 + offset dans la fenêtre
        ld     de, unpacked        ; &C000
        ld     bc, __size_music_lz ; taille COMPRESSÉE, connue au linkage
        call   depack
        ld     bc, &7F00 + __cfg_none
        out    (c), c
        ret

;--- le player, dans la banque étendue -----------------------------------
        SECTION audio, "ro"
audio_init:
        ; ...
        ret

;--- les données compressées ---------------------------------------------
        SECTION music_lz, "ro"
music_lz:
        incbin "song.bin"          ; la compression a lieu au LINKAGE

;--- la destination du dépacking, aucun octet émis -----------------------
        SECTION unpacked, "uninit"
unpacked:
        ds     &4000
```

Découper en plusieurs fichiers devient utile pour deux raisons, et deux
seulement : **ne réassembler que le player** quand on ne touche qu'à lui, et
**réutiliser** un module d'un projet à l'autre. Ce sont des raisons de
compilation séparée, pas des raisons d'architecture — d'où `PUBLIC` / `EXTERN` et
les fichiers objets, qui ne servent qu'à ce moment-là.

#### Le script de linkage

```
TARGET cpc6128 + RAM128

MEMORY_MAP {
    REGION RAM_BASE {
        PAGE 1 { SECTION main     }
        PAGE 2 { SECTION sysbank  }
        PAGE 3 { SECTION unpacked }
    }
    REGION RAM_EXP1 [PAL_PAGE 0] {
        PAGE_EXT 0 { SECTION music_lz  COMPRESS "lz48" }
        PAGE_EXT 1 { SECTION audio }
    }
}
```

`COMPRESS` est écrit **là où le placement est écrit**, jamais dans la source :
c'est une transformation qui change la taille (§8). Et `music_lz` est placée
**seule dans sa page**, donc l'adresse d'aucune autre section ne dépend de sa
taille compressée : rien à itérer.

#### Ce que le source n'écrit plus

Aujourd'hui, la même chose s'écrit avec `org b4:&4000` (`syntax.md`, §7) : la
source énonce elle-même la fenêtre, et « rien n'est déduit — une banque n'a pas
de créneau naturel ». Avec les sections, la fenêtre vient de la région, et c'est
le linker qui déduit : `audio_init` vaut `&4000 + offset` parce que la région dit
que cette page apparaît en `&4000`. Déplacer le player dans une autre page ne
touche pas une ligne de source.

### 12.3 À quoi sert concrètement la carte du §7

La carte mémoire n'est pas de la documentation : c'est la table à partir de
laquelle le linker **calcule**. Trois usages.

**1. La fenêtre donne l'`ORG`.** Une section placée dans une page dont la fenêtre
est `&4000`-`&7FFF` voit ses labels basés en `&4000`. Le `org` disparaît du
source parce que le matériel le dicte.

**2. Les valeurs de commutation deviennent des symboles.** Le linker connaît la
page `ppp` et la configuration `ccc` de chaque section, donc l'octet `11pppccc`
à sortir sur le port du PAL. Il l'expose :

| symbole | valeur, pour l'exemple du §12.2 | d'où elle vient |
|---------|--------------------------------|-----------------|
| `__cfg_audio` | `%11000101` = `&C5` | `ppp`=0, `ccc`=`%101` : page étendue 1 en `&4000` |
| `__cfg_music_lz` | `%11000100` = `&C4` | `ppp`=0, `ccc`=`%100` : page étendue 0 en `&4000` |
| `__cfg_none` | `%11000000` = `&C0` | `ccc`=`%000`, aucune extension connectée |
| `__size_music_lz` | la taille **compressée** | connue après la compression, au linkage |
| `__rom_myrom` | `15` | numéro de slot, pour le port `&DF00` |
| `__rmr_myrom` | l'octet `RMR` activant la ROM haute | assemblé depuis le profil de cible |

Le source écrit `ld bc, &7F00 + __cfg_audio`, jamais `&C5`. Le gain n'est pas
cosmétique : déplacer une section change la valeur, et une valeur écrite en dur
serait devenue fausse **en silence**. Les noms préfixés de `__` appartiennent au
linker, et sont réservés au même titre que les mots de la machine.

**3. Le profil est l'endroit unique où la polarité des bits est vérifiée.** Le
détail du décodage de `RMR` — quel bit **active** et quel bit **inhibe** la ROM
basse ou haute — est précisément le genre d'information sur laquelle les sources
de documentation se contredisent. Écrite dans le profil de cible, elle est
vérifiée **une fois**, sur machine ou sur émulateur, et tout le monde en hérite.
Recopiée dans chaque source, elle est vérifiée à chaque fois, ou jamais.

### 12.4 Combien de fichiers en sortie ? Combien de passes ?

**Le nombre de fichiers produits est une propriété de la cible, pas de la
source.** C'est la raison de fond pour laquelle le format de sortie n'a pas sa
place dans le source.

```bash
fantams game.asm -T game.ld -o game.sna     # UN fichier
```

Un snapshot est un état machine : les banques 0 à 7 sont déjà remplies dans le
dump, `audio` est *déjà* en banque 5. Rien à charger, rien à copier, aucun
loader. C'est la sortie de test, et elle est immédiate.

```bash
fantams game.asm -T game.ld -o game.dsk     # PLUSIEURS artefacts
```

Sur disquette, la même image devient un fichier par emplacement, nommé d'après
son couple (banque, adresse) :

```
B01_4000.BIN   main
B02_8000.BIN   sysbank
B04_4000.BIN   music_lz   (compressé)
B05_4000.BIN   audio
```

`unpacked` ne produit rien : son type est `"uninit"`. Et il faut alors un
**loader** — commuter la bonne configuration avant de charger chaque fichier
dans sa fenêtre, puis rendre la main. Ce loader est du code de l'auteur, dans une
section résidente — ou miroir, si aucune fenêtre ne reste en place (§13.3) ; **le linker lui fournit les nombres exacts, il ne
l'écrit pas.** Générer du code de chargement serait le premier pas vers la dérive
que ce document combat.

**Les passes** — le mot recouvre deux choses.

*Dans l'assembleur* : deux passes, toujours, quel que soit le nombre de sections.
La passe 1 fixe les adresses et collecte les symboles, la passe 2 encode. Cela
tient parce que la taille d'une instruction dépend du type de ses opérandes, pas
de leur valeur. **Jamais une troisième.**

*Dans la chaîne* : quatre étapes, une seule invocation, aucune boucle.

```
préprocesseur → assembleur (2 passes) → linker (placement, compression,
                                        résolution) → builder (conteneur)
```

L'ordre à l'intérieur du linker est ce qui évite l'itération : on place, **puis**
on compresse, **puis** on résout les symboles — possible parce que les sections
compressées sont placées de façon à ce qu'aucune adresse ne dépende de leur
taille (§8). Si une carte mémoire rendait cet ordre impossible, le linker le
**dit** et refuse, plutôt que de boucler vers un point fixe.

## 13. Autres architectures

Réponse courte : **oui, un fichier de profil est un fichier de données, et rien
n'empêche d'en écrire un.** C'est même le seul test qui prouve que le découpage
a servi à quelque chose : si décrire un ZX ou un MSX demande de toucher au code
du linker, alors le linker n'a pas de modèle, il a des cas particuliers CPC.

Mais la réponse honnête est que **le vocabulaire du §6 ne suffit pas encore** :
`PAL_PAGE` et `RMR_BIT` sont des mots CPC. Il faut les poser comme l'instance
d'un modèle générique, sans quoi le premier profil ZX les détournera.

### 13.1 Le vocabulaire générique

Trois notions, et rien de plus :

**`WINDOW`** — une plage de l'espace adressable du Z80 où quelque chose peut
apparaître. C'est elle qui donne son `ORG` à une section (§12.3).

**`BANK`** — une unité de stockage physique susceptible d'apparaître dans une
fenêtre. Attributs : taille, `ro` ou `rw`, et les fenêtres qui peuvent l'accueillir.

**`SELECT`** — comment on l'y fait apparaître : **une suite d'écritures**. C'est
le point qui décide de la généralité du modèle, car ces écritures ne sont pas du
même genre selon la machine :

```
SELECT <fenêtre>, <banque>  =  OUT  <port>, <valeur>      // CPC, ZX
SELECT <fenêtre>, <banque>  =  POKE <adresse>, <valeur>   // mappers MSX
```

Un modèle qui ne connaîtrait que `OUT` — le réflexe qu'on prend en ne regardant
que le CPC — ne pourrait pas décrire un MSX, où la commutation d'une mega-ROM
est une **écriture mémoire**. C'est la contrainte à intégrer dès maintenant,
parce qu'elle est structurante et qu'elle ne coûte rien tant que rien n'est
écrit.

S'y ajoutent des **attributs** de fenêtre ou de banque, qui sont exactement ce
que le linker sait vérifier et que l'assembleur ne peut pas connaître :

| attribut | sens | machine qui l'impose |
|----------|------|----------------------|
| `SHADOWS <fenêtre>` | commuter ici fait disparaître ce qui y était | CPC : la fenêtre `&4000` recouvre la page 1 de la RAM de base |
| `ALWAYS <banque>` | fenêtre fixe, jamais commutée | ZX 128 : `&4000` est toujours la banque 5 |
| `CONTENDED` | accès ralenti par la vidéo — le placement change le timing | ZX : `&4000`-`&7FFF` |
| `READONLY` | une écriture ici est une faute (§4.2) | toutes |
| `LOCKS` | la commutation est irréversible jusqu'au reset | ZX : bit 5 du port `&7FFD` |

### 13.2 Les trois profils, dans le même vocabulaire

**CPC 6128** — c'est le §6 et le §7, réécrits avec les mots ci-dessus : quatre
fenêtres de 16 K, huit banques par page de 64 K, un `SELECT` en `OUT` sur le port
du PAL avec la valeur `11pppccc`, et le `SHADOWS` qui porte le piège du §12.2.

**ZX Spectrum 128** — la structure est étonnamment proche du CPC : une fenêtre
commutable, huit banques candidates, un port avec un champ de bits.

```
TARGET zx128

WINDOW rom    [&0000..&3FFF]  READONLY
WINDOW low    [&4000..&7FFF]  ALWAYS bank5   CONTENDED   // porte aussi l'écran
WINDOW mid    [&8000..&BFFF]  ALWAYS bank2
WINDOW high   [&C000..&FFFF]  HOSTS bank0..bank7

SELECT high, bank<n> = OUT &7FFD, __base | <n>   // bits 0-2 : banque
                                                 // bit 3 : écran normal/shadow
                                                 // bit 4 : ROM 128/48K
                                                 // bit 5 : verrou (LOCKS)
```

Deux différences qui comptent, et que seul le profil peut porter : le port est
en **écriture seule**, sans relecture possible — c'est au source de tenir une
copie de l'état, et le profil doit le dire plutôt que de le laisser découvrir ;
et `&4000` est *contended*, donc une section de code au timing critique n'a rien
à y faire.

**MSX** — quatre pages de 16 K dont chacune choisit indépendamment quel *slot*
est visible, la sélection passant par le PPI ; et par-dessus, des mappers de
mega-ROM ou de RAM qui commutent, eux, par écriture mémoire ou par un autre port.

```
TARGET msx-konami

WINDOW page0 [&0000..&3FFF]
WINDOW page1 [&4000..&7FFF]
WINDOW page2 [&8000..&BFFF]
WINDOW page3 [&C000..&FFFF]

SELECT page<p>, slot<s>       = OUT  &A8, ...        // 2 bits par page
SELECT page2,   rombank<n>    = POKE &8000, <n>      // mapper : écriture MÉMOIRE
```

C'est ce profil qui justifie `POKE`, et il en tire un piège propre : puisque la
commutation est une écriture, **écrire une donnée dans la plage d'un mapper
commute par accident**. Le linker peut le signaler ; l'assembleur n'en saura
jamais rien.

> **À vérifier avant d'écrire ces profils.** Les valeurs citées ici — champs de
> bits du port `&7FFD`, port PPI `&A8`, adresses de commutation des mappers MSX,
> et jusqu'aux modèles concernés — sont restituées de mémoire. Elles illustrent
> la *forme* du modèle, pas des chiffres à recopier : chacune doit être vérifiée
> sur la documentation de la machine, et le §12.3 dit pourquoi c'est justement
> l'intérêt de les écrire dans un profil unique.

### 13.3 La continuité à travers une commutation

Le recouvrement du §12.2 — du code à `&4000` qui disparaît quand on commute une
banque à `&4000` — n'est pas une bizarrerie CPC. Mais sa formulation évidente,
« une routine ne peut pas s'exécuter dans la fenêtre qu'elle commute », est
**fausse**, et le contre-exemple n'est pas exotique : sur une mega-ROM MSX, les
quatre fenêtres peuvent être des banques de ROM, il n'existe aucun résident où se
réfugier, et la solution universelle est de **répliquer le stub de commutation au
même offset dans toutes les banques**. Le PC continue alors sur les mêmes octets.
C'est la pratique standard sur toute architecture à cartouche.

Le CPC a le luxe d'une page toujours résidente ; prendre ce luxe pour une loi
donne une règle qu'il faudrait trouer dès le deuxième profil.

#### L'invariant, et ses trois pointeurs

Ce n'est pas une contrainte de placement, c'est une contrainte de continuité :

> Au moment de la commutation, l'octet que le PC lira ensuite doit être le même
> octet — ou porter la même instruction.

Le placement hors fenêtre n'est qu'un moyen de la satisfaire, le plus grossier :
la continuité y est triviale puisque rien ne change. La réplication en est un
autre, tout aussi valide.

Et posé ainsi, l'invariant se dédouble : **le PC n'est pas le seul pointeur qui
traverse une commutation.**

| pointeur | ce qui casse | où ça se paie |
|----------|--------------|---------------|
| **PC** | le flux d'instructions saute dans du vide | CPC : fenêtre `&4000` ; MSX : les quatre |
| **SP** | le `ret` suivant part n'importe où | CPC config `%010` (les 64 K basculent), modes *all-RAM* du +3 |
| **le vecteur d'interruption** | une interruption tombée juste après va chercher son handler dans la banque entrante | toutes, dès que les interruptions sont actives |

Le cas SP est le plus fréquent et le moins diagnostiqué : ça marche jusqu'au
premier `call` imbriqué. Le contrôle est le même que pour le PC, à condition que
le profil sache où la pile est placée.

#### Ce qu'une machine prouve, et ce qu'elle se contente de croire

« Identique ou compatible » couvre trois échelons qui n'ont pas le même statut, et
qu'il faut séparer.

**1. L'identité par construction — une section miroir.** Le bon geste n'est pas
de vérifier après coup que deux banques portent les mêmes octets : c'est de
rendre l'écart impossible. Une **section miroir** est placée par le linker au
même offset dans N banques, et il en émet N copies. Écrite une fois dans le
source, dupliquée par le placement — ce n'est pas un attribut de section mais un
**troisième genre de placement**, d'où sa place au linker, comme la compression.

La règle devient alors vérifiable sans jugement :

> Commuter depuis une fenêtre est licite si le code commutant est hors de cette
> fenêtre, ou dans une section miroir couvrant toutes ses banques candidates.

**2. L'accord structurel au même offset.** C'est la forme réelle du
« compatible » dans la nature : chaque banque porte à son offset 0 un
`jp <entrée de la banque>`. Les octets diffèrent — les cibles ne sont pas les
mêmes — mais toutes les banques ont bien quelque chose de la bonne taille au bon
endroit. Encore mécaniquement vérifiable : « toutes les banques candidates ont
une section placée à cet offset exact, de cette taille exacte ». Échelon
intermédiaire, et probablement le plus employé.

**3. L'équivalence libre.** « Ces octets diffèrent mais font la même chose. »
Le prouver, c'est prouver l'équivalence de deux programmes : indécidable. Le
linker ne l'essaie pas, ni par heuristique ni par analyse. Il ne peut que
recevoir une **assertion de l'auteur**, et le seul enjeu est de la tenir étroite :
bornée à une plage d'adresses nommée, jamais un drapeau qui éteint le contrôle
pour toute une section — sinon il est activé partout et le contrôle meurt.

La ligne est donc nette : **le linker vérifie l'identité et l'accord structurel ;
il ne fait que consigner l'équivalence.** Et il refuse par défaut, seul état sûr
puisque la faute est indétectable à l'exécution : la machine ne plante pas au
basculement, elle plante trois instructions plus loin.

#### Deux conséquences pratiques

**Le miroir a un coût que seul le linker peut chiffrer.** Vingt octets × 8
banques, c'est gratuit ; deux kilo-octets de bibliothèque résidente × 8, c'est
16 K de RAM étendue partis en copies. Le linker connaît le produit et le dit —
`miroir : 20 octets × 8 banques = 160 octets`. C'est le nombre dont l'auteur a
besoin pour arbitrer, et personne ne le calcule à la main.

**Le miroir résout aussi le loader du §12.4**, qui doit rester présent quelle que
soit la configuration. Même besoin, même mécanisme : deux problèmes qui
semblaient distincts n'en font qu'un.

#### Un contrôle, valable partout

Sur CPC la question se pose une fois, sur MSX quatre — une par fenêtre. Le
contrôle du linker est **le même sur les trois machines** dès lors que le profil
déclare quelle fenêtre recouvre quoi, où la pile est placée et où va le vecteur
d'interruption. Un contrôle écrit une fois, valable partout : c'est le retour sur
investissement du modèle générique, et l'argument pour ne pas laisser les mots
CPC s'installer dans le vocabulaire.

### 13.4 Ce qui ne change pas : les sections

Le point important de la réponse est ce qu'il n'y a **rien** à faire.

Les sections ne changent pas d'une machine à l'autre. Les trois types — `"ro"`,
`"rw"`, `"uninit"` — sont universels : ils décrivent la nature du contenu, pas la
machine. `BOUNDARY`, `PUBLIC`, `EXTERN`, `PHASE`, les plafonds de taille, la
détection d'écriture en ROM : rien là-dedans ne connaît le CPC. Une source qui
n'écrit pas de valeur matérielle en dur est **portable telle quelle**, et le seul
morceau à réécrire par machine est la section qui commute — le `sysbank` du
§12.2.

Ce n'est pas un effet secondaire heureux : c'est la couture de portabilité, et
elle est visible dans l'exemple. `sysbank` existait pour contourner un
recouvrement CPC ; il se trouve qu'il est aussi, exactement, le seul fichier à
porter d'une machine à l'autre.

## Références

- Format CRO (Longshot / Logon System) —
  <https://github.com/Logon-System/CRO-Format/blob/main/README.md>
- Guide CRO —
  <https://github.com/Logon-System/CRO-Format/blob/main/doc/GuideCRO_ENG.md>
