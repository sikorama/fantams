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

Il existe des chaînes complètes — assembleur, linker, générateur d'images de
support et moteur d'encodage — entièrement pilotées depuis le source. Les
directives qui les caractérisent sont nommables, et ce sont elles qui montrent où
passe la frontière. Les familles à écarter :

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

- **RAM de base** — 64 K, quatre banques de 16 K.
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

Les trois types décrivent la **nature du contenu**. Ce qu'ils ne décrivent pas,
c'est une relation entre deux sections : « ce `rw` est initialisé depuis ce
`ro` ». C'est un besoin réel — le couple `_INITIALIZED` / `_INITIALIZER` de SDCC,
et tout code qui vit en ROM et tourne en RAM — et il se dit par `INIT_FROM`,
sans quatrième type (§13.4).

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

Quatre blocs :

1. **Le flux d'octets** — code et données bruts, par section.
2. **La table des sections** — nom, type, taille, taille maximale déclarée.
3. **La table des symboles et des relocalisations** — pour chaque symbole : sa
   section, son offset, sa portée (`PUBLIC`, `EXTERN`, local).
4. **La table des accès à adresse littérale** — chaque `ld (nn),a`, `in a,(n)`,
   `out (n),a` dont l'adresse est écrite en clair, avec son offset et son sens.
   L'assembleur ne l'interprète pas : il ne connaît aucune machine (§1), il
   **consigne**. C'est le linker, qui connaît le profil, qui y lit une écriture
   dans une plage commutant par accident ou une lecture d'un port en écriture
   seule (§13.1). Même partage des rôles que la relocalisation : l'assembleur
   note, le linker tranche.

Un `ld (hl),a` dont `HL` est calculé n'y figure pas et ne sera **jamais**
attrapé. C'est la limite du contrôle, et elle est à écrire plutôt qu'à
découvrir.

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

> Émettre à la suite si le bloc tient entièrement dans la page de 256 octets
> courante ; sinon sauter au début de la suivante.

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

Le vocabulaire employé ici est le vocabulaire générique du §13.1 —
`WINDOW`, `BANK`, `CONFIG`, `SELECT` —, le même sur les trois machines. Ce qui
est CPC dans ce qui suit, ce sont les **noms** et les **nombres** ; ils viennent
du profil de cible, jamais du code du linker.

Trois règles que les blocs ci-dessous appliquent, et que le §13.1 justifie :

- **la taille d'une banque est déclarée**, jamais déduite de la fenêtre où elle
  apparaît ;
- **le placement se dit dans une configuration**, pas dans une fenêtre : c'est
  la configuration qui sait quelle banque apparaît où — parce que sur la plupart
  des machines les fenêtres ne se choisissent pas indépendamment ;
- **les attributs se portent là où le matériel les porte** : la contention et la
  visibilité vidéo sont des propriétés de banque, pas de fenêtre.

Et une frontière qu'il faut tracer avant d'écrire une ligne : le **profil de
cible** décrit la machine, et il est intégré au binaire (§9) ; le **script de
linkage** ne dit que ce que le profil ne peut pas savoir — quelle section va où.
Les deux blocs qui suivent ne s'écrivent ni au même moment, ni par la même
personne, et c'est le premier qui contient la quasi-totalité du matériel.

Le profil, tel qu'un `--target cpc6128+ram128` l'apporte :

```
// --- Les fenêtres : une grille de quatre, de 16 K -----------------------
WINDOW w0 [0x0000..0x3FFF]
WINDOW w1 [0x4000..0x7FFF]
WINDOW w2 [0x8000..0xBFFF]
WINDOW w3 [0xC000..0xFFFF]

// --- Les banques : taille déclarée, attributs portés ici ----------------
BANK base0..base3  SIZE 0x4000  rw  VIDEO   // les seules que le CRTC sait lire
BANK ext0..ext3    SIZE 0x4000  rw          // page étendue : pas de VIDEO
BANK rom_lo        SIZE 0x4000  ro
BANK rom_hi<n>     SIZE 0x4000  ro          // n : numéro de ROM haute

// --- Les configurations : ce que le matériel sait réellement faire ------
CONFIG SET ram {
    linear     [CODE %000] { w0 base0  w1 base1   w2 base2  w3 base3 }
    ext_high   [CODE %001] { w0 base0  w1 base1   w2 base2  w3 ext3  }
    all_ext    [CODE %010] { w0 ext0   w1 ext1    w2 ext2   w3 ext3  }
    shifted    [CODE %011] { w0 base0  w1 base3   w2 base2  w3 ext3  }
    ext_w1<b>  [CODE %1bb] { w0 base0  w1 ext<b>  w2 base2  w3 base3 }
}
SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE
SELECT ram.all_ext  STACK OUTSIDE [0x0000..0xFFFF]   // les quatre basculent

// --- Les ROMs : deux axes indépendants, qui recouvrent en LECTURE -------
CONFIG SET rom_lower OVER ram { off { }  on { w0 rom_lo    } }
CONFIG SET rom_upper OVER ram { off { }  on { w3 rom_hi<n> } }

// Polarité écrite ici, et ici seulement : 0 active, 1 inhibe (§7, §12.3)
SELECT rom_lower = OUT 0x7F00, RMR.BIT2 = (on ? 0 : 1)
SELECT rom_upper = OUT 0x7F00, RMR.BIT3 = (on ? 0 : 1)
                   OUT 0xDF00, n
```

Le script, tel que l'auteur l'écrit — et le §12.2 montre qu'il tient en dix
lignes pour un programme banqué réel :

```
TARGET cpc6128 + RAM128

MEMORY_MAP {
    CONFIG linear {
        w1 { SECTION main     }
        w2 { SECTION sysbank  }
        w3 { SECTION unpacked }
    }
    CONFIG ext_w1<1> { w1 { SECTION audio } }

    // Une ROM de 16 K découpée en deux blocs de 8 K : c'est un découpage de
    // placement à l'intérieur d'une banque, pas une banque de 8 K (§13.1).
    CONFIG rom_upper.on, ROM 15 {
        w3 [OFFSET 0x0000, SIZE 0x2000] { SECTION audio_rom     }
        w3 [OFFSET 0x2000, SIZE 0x2000] { SECTION graphics_data
                                          SECTION menu_text     }
    }
}

OUTPUT_FORMAT {
    TARGET      = "SNA_V2"
    ENTRY_POINT = 0x8000                 // le `run` du source, s'il n'est pas ici
    STACK       = [0x3F00..0x3FFF]       // une plage, pas une adresse
    INT_VECTOR  = 0x0038                 // déclaré, jamais déduit
    // ou, pour une ROM :
    // TARGET = "CRO"  ;  CRO_ROM_NUMBER = 15
}
```

Trois valeurs qui ne sont pas du décor : le §13.3 en fait dépendre un contrôle
de correction.

**`ENTRY_POINT`** est ce que `run` écrit dans le source. `run` reste licite —
le §12.1 est un engagement de compatibilité, et une source d'un `org` et d'un
`run` ne doit pas gagner un fichier de script — mais il devient une **donnée
transmise** au builder, non une directive de format : il n'échappe pas au §2, il
en relève. Le script l'emporte s'il la nomme aussi.

**`STACK` est une plage**, parce que `SP` bouge et qu'une adresse unique ne dit
rien de vrai d'un programme qui empile. Elle est déclarée dans le script et non
dans le profil : la pile est posée par le programme, pas par la machine. Un
contrôle qui reposerait sur une valeur que le linker ne peut pas voir est un
contrôle qui ne se déclenche jamais.

**`INT_VECTOR` est déclaré, jamais déduit.** En mode 2 il dépend du registre `I`
et d'une table construite à l'exécution : hors de portée d'une analyse statique.

Le linker peut alors signaler `2048 bytes unused at 0xE000 in rom_hi15`, ou y
loger une section marquée comme déplaçable.

Trois choses que ni le profil ni le script **n'écrivent**, et c'est le test du
modèle. Aucun des deux ne déclare que commuter `ext_w1<1>` fait disparaître
`main` : cela se **calcule** en comparant `linear` et `ext_w1<1>` sur `w1`
(§13.1). Aucun ne déclare que `w0` est en lecture seule quand `rom_lower` est
sur `on` : c'est la banque qui est `ro`, la configuration dit seulement qu'elle
est là. Et aucun n'écrit la valeur `%11000101` que le source sortira sur le
port : elle se calcule, et devient le symbole `__cfg_audio` (§12.3).

## 7. Ce que le linker doit savoir du matériel

Ce paragraphe ne porte **aucune valeur**. Il énumère les *natures*
d'information qu'un profil de cible doit savoir dire ; les chiffres vivent à
deux endroits, et deux seulement :

- `docs/recherche/*.md` — les valeurs vérifiées sur sources primaires, citées et
  arbitrées, avec les contradictions restées ouvertes signalées comme telles ;
- les profils de cible eux-mêmes — les valeurs *exécutables*, celles que le
  linker lit.

C'est une décision, et elle vient d'une erreur. La version précédente de ce
paragraphe recopiait la carte du CPC dans la spec, et c'est précisément là que
deux fautes se sont installées : la polarité des bits de ROM du registre `RMR`
était inversée — 0 active, 1 inhibe, sept sources concordantes — et `ccc = 000`
y était décrit comme « aucune RAM étendue connectée » alors que c'est la
disposition **linéaire par défaut**, indépendamment de la présence d'une
extension. Le §12.3 soutient qu'une valeur matérielle doit être vérifiée en un
seul endroit ; ce paragraphe l'a démontré à ses dépens. **Une valeur écrite deux
fois est une valeur fausse une fois.**

| nature de l'information | porteur (§13.1) | exemple de ce que ça vaut sur une machine |
|---|---|---|
| grilles de fenêtres, et leurs bornes | `WINDOW` | quatre de 16 K sur CPC et ZX ; sur MSX, quatre de 16 K **et** quatre de 8 K superposées |
| inventaire des banques, et la **taille** de chacune | `BANK` | 16 K de RAM sur CPC ; 8 K pour une mega-ROM Konami |
| lecture seule matérielle | `BANK` | une ROM |
| accès ralenti par la vidéo | `BANK` | ZX : certaines banques RAM, et non certaines adresses |
| visibilité par le contrôleur vidéo | `BANK` | CPC : la RAM étendue n'est **jamais** lue par le CRTC — un refus de placement |
| états de carte réellement atteignables | `CONFIG` | CPC : huit ; +2A/+3 en mode spécial : quatre |
| ce que masque une commutation | *calculé* depuis les `CONFIG` | le piège du §12.2 |
| mécanisme de sélection, et ses champs de bits | `SELECT` | `OUT` sur un port ; écriture mémoire sur un mapper |
| dépendance du port au numéro de banque | `SELECT` | CPC au-delà de 512 K : une partie du numéro est dans l'adresse du port |
| préconditions et séquences | `SELECT` | un registre subordonné à un bit d'un autre ; un déverrouillage préalable |
| contraintes de commutation vérifiables | `SELECT` | fenêtre d'exécution interdite, interruptions, pile, plage commutant par accident |
| irréversibilité | pagination entière | ZX : un verrou qui bloque *tous* les ports de pagination jusqu'au reset |
| où sont la pile et le vecteur d'interruption | **script** (`STACK`, `INT_VECTOR`) | ce qui rend calculable le contrôle du §13.3 — et ce n'est pas dans le profil, parce que c'est le programme qui les pose, pas la machine |

Aucune de ces lignes n'est propre au CPC : c'est la liste que le §13.2 remplit
trois fois.

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
peut itérer, mais il refuse bruyamment la non-convergence.** Un outil qui itère
vers un point fixe sans exiger d'y arriver ne signale rien quand il n'y arrive
pas : il rend un binaire *apparemment* correct, et la faute se découvre à
l'exécution. Une itération sans critère d'arrêt prouvé n'est pas une commodité,
c'est une faute silencieuse de plus.

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

**Statut : objectif documenté, non engagé.** Ce qui suit est vérifié (sources
dans `docs/recherche/sdcc-objets-rel.md`) et sert à une seule chose : ne pas
prendre aujourd'hui, dans le modèle d'objet, une décision qu'il faudrait défaire
le jour où on s'y engagerait. L'étage correspondant vient **après l'étage C2**
(§10), et rien n'oblige à le franchir.

Le sens est fixé : c'est **le linker de fantams qui lirait les objets de SDCC**,
non l'inverse. Émettre du `.rel` pour se faire lier par `sdld` nous soumettrait
à ses limites — format V3 seul, trois bits d'attribut d'area, aucun alignement,
et l'ordre des areas déterminé par l'ordre d'apparition dans les objets.

Et le manque côté SDCC n'est pas un « point faible » : **il n'y a rien**. La
directive `.bank` est commentée dans `sdasz80`, `newbank()` n'est appelé de
nulle part, l'aide de `sdldz80` n'offre aucune option de bank, et le seul
banking implémenté est câblé pour la Game Boy sous garde `TARGET_IS_GB`. Un
linker qui connaît la pagination d'une machine Z80 réelle est donc exactement ce
qui manque à cette chaîne.

**Le prix, chiffré.** Lire le dialecte `sdas` du format ASxxxx **V3** — pas « le
format ASxxxx » ; valider la ligne de format `[XDQ][HL][234]` au lieu de
supposer `XL4`, la largeur d'adresse étant passée de 24 à 32 bits dans une
version *mineure* (4.4.1) ; tolérer le champ `addr` de la ligne `A` et
l'enregistrement `O` (`.optsdcc`) ; implémenter l'échappement `0xfX` ;
**fabriquer les symboles `s_<area>` / `l_<area>`**, sans quoi `crt0.rel` ne se
lie pas ; lire l'ABI dans `.optsdcc` pour refuser un mélange `sdcccall(0)` /
`sdcccall(1)` ; et accepter que les noms d'areas soient insensibles à la casse
quand les symboles y sont sensibles.

**Ce qui est acquis en échange.** Le banking SDCC est déjà exprimable **sans
extension du format** : le nom d'area (`_CODE_<n>`) et le symbole absolu
`b_<fonction>` portent l'information « quelle area dans quelle banque ».

**Les deux conséquences pour le modèle d'objet, à ne pas rater maintenant.**

1. Une section ≈ une *area*, et le modèle **peut** être plus riche que `.rel` :
   la contrainte de pauvreté ne vaudrait que pour l'émission, qui n'est pas au
   programme.
2. La projection des types n'est pas celle qu'on croit. `_BSS` n'existe pas
   côté SDCC/Z80 — area vestigiale déclarée dans `crt0.s`, jamais alimentée par
   le compilateur. La correspondance réelle est : `ro` → `_CODE` (`_HOME`,
   `_CABS`) ; `uninit` → `_DATA`, mis à zéro par `crt0` ; et un `rw` **initialisé**
   → le couple `_INITIALIZED` (destination RAM) / `_INITIALIZER` (image ROM),
   que ni `ro` ni `rw` ne décrit seul. C'est ce couple qui motive la relation
   `INIT_FROM` du §13.4 — et elle ne coûte rien à poser tout de suite.

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

### Les étages

| étage | ce qu'on gagne | relocalisation |
|-------|----------------|----------------|
| **A.** `SECTION` interne, placement toujours absolu (`org` à l'intérieur) | plafond de taille, détection d'écriture en `"ro"`, section dans la table des symboles | non |
| **B.** fichier objet et expressions relocalisables | compilation séparée, `PUBLIC` / `EXTERN`, tailles résolues au linkage (§8) | oui — le gros morceau |
| **C1.** le linker qui **place et calcule** : fenêtres, banques, configurations, `ORG` déduit, symboles de commutation, chevauchements inter-sections, compression | le maillon 2 du §3.2 : un programme banqué devient constructible | — |
| **C2.** le linker qui **vérifie** : continuité et ses trois pointeurs (§13.3), sections miroir, `CLOBBERS`, `INIT_FROM` | un programme banqué **faux** devient refusable | — |
| **D.** *(non engagé)* lecture des objets `.rel` de SDCC | interopérabilité C, au prix chiffré au §9 | — |

C1 et C2 sont deux étages et non un, pour la raison qui fait de l'étage A un
investissement autonome : C1 produit un binaire, C2 refuse un binaire faux, et
les mêler garantit qu'on livrera C1 en promettant C2. C2 attrape en outre des
fautes que rien d'autre n'attrape — la faute de continuité est indétectable à
l'exécution, la machine ne plantant pas au basculement mais trois instructions
plus loin.

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
dans le profil de cible — un seul endroit, vérifiable (§7) — et non masqués
derrière une macro opaque comme `{PAGESET}`.

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

| section | type | banque | fenêtre | configuration |
|---------|------|--------|---------|---------------|
| `main` | `"ro"` | `base1` | `&4000`-`&7FFF` | `linear` |
| `sysbank` | `"ro"` | `base2` | `&8000`-`&BFFF` | toutes |
| `audio` | `"ro"` | `ext1` | `&4000`-`&7FFF` | `ext_w1<1>` |
| `music_lz` | `"ro"`, compressée | `ext0` | `&4000`-`&7FFF` | `ext_w1<0>` |
| `unpacked` | `"uninit"` | `base3` | `&C000`-`&FFFF` | `linear` |

Le piège est dans les deux dernières colonnes : les configurations `ext_w1<b>`
donnent `w1` à une banque étendue **à la place** de `base1`. Or `main` est
justement dans `base1`. Commuter `audio` fait donc disparaître `main` sous ses
propres pieds.

Ce n'est pas une déclaration du profil : le linker le **calcule** en comparant
`linear` et `ext_w1<1>` sur `w1` (§13.1). Aucun `SHADOWS` n'a été écrit, et il
n'y avait donc aucune occasion de l'écrire faux.

C'est pour cela que `sysbank` existe : la commutation, l'appel au player et le
dépacking vivent dans `base2`, qu'aucune de ces configurations ne recouvre.
**Cette contrainte, l'assembleur ne peut pas la connaître — elle est dans les
configurations de la machine. Le linker, si.** Il refuse un appel depuis `main`
vers `audio`, en nommant les deux sections et la fenêtre qu'elles partagent,
plutôt que de laisser produire un programme qui se sabote à la première
commutation. Le §13.3 donne la règle générale dont ce refus n'est qu'un cas, et
la seconde façon de le satisfaire quand aucune fenêtre ne reste résidente.

Deux configurations de plus mériteraient un mot, et le profil les porte sans que
l'exemple les emploie : `all_ext` bascule les quatre fenêtres d'un coup — donc
aussi la pile —, et aucune banque `ext<n>` ne porte l'attribut `VIDEO`, ce qui
interdit d'y placer une section écran (§13.1).

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
        ld     bc, __port_ram_audio + __val_ram_audio   ; port ET valeur : §12.3
        out    (c), c
        call   audio_init          ; vaut &4000 + offset : le linker le sait
        ld     bc, __port_ram_linear + __val_ram_linear
        out    (c), c
        ret

sysbank_unpack:
        ld     bc, __port_ram_music_lz + __val_ram_music_lz
        out    (c), c
        ld     hl, music_lz        ; &4000 + offset dans la fenêtre
        ld     de, unpacked        ; &C000
        ld     bc, __size_music_lz ; taille COMPRESSÉE, connue au linkage
        call   depack
        ld     bc, __port_ram_linear + __val_ram_linear
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
    CONFIG linear {
        w1 { SECTION main     }
        w2 { SECTION sysbank  }
        w3 { SECTION unpacked }
    }
    CONFIG ext_w1<0> { w1 { SECTION music_lz  COMPRESS "lz48" } }
    CONFIG ext_w1<1> { w1 { SECTION audio } }
}
```

Le placement se dit **dans une configuration**, et la fenêtre en découle : c'est
elle qui donnera son `ORG` à la section. Écrire `w1 { SECTION audio }` sous
`ext_w1<1>` dit d'un seul geste « dans la banque `ext1` » et « vue en `&4000` »,
sans que la source ait à le savoir.

`COMPRESS` est écrit **là où le placement est écrit**, jamais dans la source :
c'est une transformation qui change la taille (§8). Et `music_lz` est placée
**seule dans sa configuration**, donc l'adresse d'aucune autre section ne dépend
de sa taille compressée : rien à itérer.

#### Ce que le source n'écrit plus

Aujourd'hui, la même chose s'écrit en nommant le rangement dans la source, banque
par banque, et « rien n'est déduit — une banque n'a pas de créneau naturel »
(`syntax.md`, §7, et l'ADR 0005). Avec les sections, la fenêtre vient de la
configuration, et c'est le linker qui déduit : `audio_init` vaut `&4000 + offset`
parce que la configuration dit que `ext1` y apparaît. Déplacer le player dans une
autre banque ne touche pas une ligne de source.

C'est le renversement complet de la question que l'ADR 0005 tranchait : il y
avait à choisir *comment le source nomme un emplacement de rangement*, avec tout
ce que cela traînait — le masquage, la rémanence, sur quel paramètre d'`ORG` le
préfixe se porte. Au niveau du linker, cette question ne se pose plus, parce que
le source ne nomme plus d'emplacement du tout : il nomme une section. Le
vocabulaire du §13.1 ne reprend donc rien de cette notation, et n'a pas à rester
compatible avec elle.

L'ADR 0005 continue de décrire ce que le code fait aujourd'hui ; c'est l'étage C1
(§10) qui le remplacera, et c'est à ce moment-là que son statut sera à revoir.

### 12.3 À quoi sert concrètement le profil de cible

Le profil n'est pas de la documentation : c'est la table à partir de laquelle le
linker **calcule**. Trois usages.

**1. La fenêtre donne l'`ORG`.** Une section placée dans une configuration voit
ses labels basés à l'adresse de la fenêtre où cette configuration fait apparaître
sa banque — `&4000` pour `audio`. Le `org` disparaît du source parce que le
matériel le dicte.

**2. Les valeurs de commutation deviennent des symboles.** Le linker connaît la
page `ppp` et la configuration de chaque section, donc l'octet `11pppccc` à
sortir sur le port du PAL. Mais **un symbole ne peut pas être « l'octet »** : sur
ZX, le port `&7FFD` porte quatre axes à la fois — banque, écran affiché, numéro
de ROM, verrou — et y sortir la seule valeur de l'axe de pagination écraserait
les trois autres, silencieusement, qui est précisément la faute que ce paragraphe
combat. Et sur CPC au-delà de 512 K, ce n'est même plus la valeur qui varie mais
l'**adresse du port**.

Le linker expose donc un **triplet par axe**, jamais un octet global :

| symbole | ce que c'est | valeur, pour l'exemple du §12.2 |
|---------|--------------|---------------------------------|
| `__port_<axe>_<config>` | l'adresse d'écriture — port pour un `OUT`, adresse mémoire pour un `POKE`. Indexée par la configuration parce qu'elle peut en dépendre. | `__port_ram_audio` = `&7F00` |
| `__val_<axe>_<config>` | la valeur à écrire, **bornée aux bits de l'axe** | `__val_ram_audio` = `%11000101` = `&C5` ; `__val_ram_music_lz` = `&C4` ; `__val_ram_linear` = `&C0` |
| `__mask_<axe>` | les bits du port qui appartiennent à l'axe, pour que le source écrive `(état & ~__mask) \| __val` sans toucher aux autres | sans objet sur l'axe `ram` du CPC, indispensable sur `&7FFD` |

S'y ajoutent les symboles qui ne relèvent d'aucun axe :

| symbole | valeur | d'où elle vient |
|---------|--------|-----------------|
| `__size_music_lz` | la taille **compressée** | connue après la compression, au linkage |
| `__romnum_myrom` | `15` | numéro de ROM haute : la **seconde** écriture d'un `SELECT` qui en compte deux (`&DF00`) |
| `__off_<section>` | l'offset dans sa banque | pour un loader, ou une recopie |

Le source écrit `ld bc, __port_ram_audio + __val_ram_audio`, jamais `&7F00 +
&C5`. Le gain n'est pas cosmétique : déplacer une section change la valeur, et
une valeur écrite en dur serait devenue fausse **en silence**. Les noms préfixés
de `__` appartiennent au linker, et sont réservés au même titre que les mots de
la machine.

Ce que le linker ne fournit **pas** : la copie de l'état. Un port en écriture
seule oblige le source à tenir en RAM la dernière valeur écrite — c'est de la
RAM, donc du ressort de l'auteur, au même titre que le loader du §12.4. Le
linker donne `__mask_<axe>` pour que cette copie se mette à jour sans écraser
les axes voisins ; il ne l'alloue pas.

Le symbole `__val_ram_linear` mérite son nom : la version précédente de ce
document l'appelait `__cfg_none` et le glosait « aucune extension connectée ».
C'est faux — `ccc = 000` est la disposition linéaire par défaut, extension
présente ou pas — et l'erreur venait de la carte recopiée dans la spec. C'est
exactement l'argument de l'usage 3.

**3. Le profil est l'endroit unique où une valeur matérielle est vérifiée.** Une
partie de ce que le linker doit savoir est *contredite par la littérature*, y
compris entre une source constructeur et le silicium. Écrite dans le profil, une
telle valeur est vérifiée **une fois**, sur machine ou sur émulateur, et tout le
monde en hérite. Recopiée dans chaque source, elle est vérifiée à chaque fois, ou
jamais.

Trois contradictions réelles, relevées et non levées dans
`docs/recherche/cpc-gate-array-rmr.md` :

- **l'effet du bit 4 de `RMR`** : le manuel Amstrad (SOFT968) dit qu'écrire 1
  efface *le bit de poids fort* du diviseur par 52 ; Grimware, Cpctech et Logon
  disent qu'il remet *le compteur entier* à zéro. Les deux ne peuvent pas être
  vraies, et ici la source de niveau 1 est la moins fiable — l'effet observé
  « l'interruption arrive 52 lignes plus loin » n'est explicable que par la
  seconde lecture.
- **le décodage du port du PAL** : A15 = 0 seul, ou A15 = 0 **et** A14 = 1 selon
  les sources, l'une d'elles se contredisant d'une page à l'autre. Sans
  conséquence si l'on écrit sur `&7F00` — et c'est justement le genre de « sans
  conséquence si » qu'un profil doit fixer une fois.
- **le nombre de bits de page réellement décodés** : 2 bits (256 K), 3 bits
  (512 K), ou zéro sur un 6128 nu.

Ce que ce paragraphe affirmait auparavant — que la polarité des bits de ROM
serait « précisément le genre d'information sur laquelle les sources se
contredisent » — était faux : sept sources indépendantes disent toutes que 0
active et 1 inhibe, sans exception. Le piège y est de **nommage**, non de valeur :
un registre intitulé « ROM *enable* register » dont les bits s'appellent « ROM
*disable* ». L'argument tenait, l'exemple non.

#### Qui vérifie, et comment on le prouve

« Vérifiée une fois, sur machine ou sur émulateur » est l'argument central de ce
paragraphe, et **rien ne l'incarne encore** : les valeurs des dossiers de
`docs/recherche/` sont vérifiées sur *documentation*, ce qui est un cran en
dessous, et les trois contradictions ci-dessus y sont explicitement laissées
ouvertes.

D'où un troisième genre d'artefact, à côté du cas de référence : une **source de
vérification**, versionnée, minuscule et autonome, une par valeur litigieuse.
Elle ne se compare pas à des octets attendus — son juge est la machine. Elle rend
un résultat observable (un octet à l'écran, un compteur, une durée), et son
verdict remonte dans le dossier de recherche avec sa date et le modèle exact
employé.

Ce n'est pas un raffinement : c'est ce qui transforme « non tranché par mesure
dans le cadre de cette recherche » — phrase qui revient trois fois dans les
dossiers — en dette nommée plutôt qu'en note de bas de page. Et un profil de
cible peut alors distinguer, par valeur, ce qui est *attesté par la
documentation* de ce qui est *mesuré ici, sur telle machine, à telle date*.

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
section résidente — ou miroir, si aucune fenêtre ne reste en place (§13.3) ; **le
linker lui fournit les nombres exacts, il ne l'écrit pas.** Générer du code de
chargement serait le premier pas vers la dérive que ce document combat.

Ce n'est pas la même chose qu'un refus définitif : proposer plus tard un outil,
une option ou un canevas de loader à recopier reste envisageable, et se décidera
sur pièces. Ce qui est arrêté ici, c'est que **le linker n'en dépend pas** — ni
pour placer, ni pour vérifier. Un générateur qui deviendrait la seule façon de
produire un programme banqué aurait ramené dans la chaîne ce que le découpage en
a sorti.

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

C'est aussi ce qui a fixé le vocabulaire employé depuis le §6. Une première
version de ce document écrivait `PAL_PAGE` et `RMR_BIT` dans le script de
linkage : des mots CPC posés à l'endroit du modèle, que le premier profil ZX
aurait détournés. Ce chapitre pose donc les notions d'abord, et les trois profils
ensuite — c'est l'ordre inverse de celui où le document a été écrit, et le seul
qui tienne.

### 13.1 Le vocabulaire générique

Quatre notions, et rien de plus.

**`WINDOW`** — une plage de l'espace adressable du Z80 où quelque chose peut
apparaître. C'est elle qui donne son `ORG` à une section (§12.3). Un profil peut
en déclarer **plusieurs grilles superposées** : sur MSX, les quatre pages de
slot de 16 K couvrent `&0000`-`&FFFF` pendant que les fenêtres de mapper de 8 K
découpent `&4000`-`&BFFF`, et les deux découpages sont actifs en même temps.

Deux grilles qui se recouvrent posent une question qu'il faut trancher avant
d'écrire un profil : **laquelle donne l'`ORG` ?** La réponse est qu'il n'y a
jamais d'arbitrage — l'`ORG` vient de la fenêtre de **la grille à laquelle
appartient la banque de la section**. Une section placée dans un segment de
mapper de 8 K est basée par la fenêtre de mapper, pas par la page de slot qui la
contient, même quand les deux commencent à la même adresse.

Et le recouvrement devient un **refus calculé**, jamais déclaré, sur le modèle de
`SHADOWS` : deux sections placées dans deux grilles différentes dont les fenêtres
se recouvrent dans un même état sont un conflit que le linker nomme. C'est la
mécanique du §12.2 appliquée à un axe de plus, et elle ne coûte pas un mot de
vocabulaire.

**`BANK`** — une unité de stockage physique susceptible d'apparaître dans une
fenêtre. **Sa taille est déclarée, jamais implicite** : 16 K de RAM sur CPC, 8 K
pour une mega-ROM Konami ou ASCII8, 16 K pour ASCII16, 8 K pour une ROM
Multiface. Un 16 K câblé dans le linker suffirait à rendre un MSX indescriptible
— et « décrire une machine ne doit pas demander de toucher au code du linker »
est le seul test qui prouve que ce découpage a servi.

Découper une banque de 16 K en deux blocs de 8 K reste possible, mais c'est un
**découpage de placement à l'intérieur d'une banque** (`OFFSET`, `SIZE`), et non
une banque de 8 K : les deux moitiés apparaissent ensemble ou pas du tout.

**`CONFIG`** — un état de carte nommé : pour les fenêtres qu'il concerne, quelle
banque y apparaît. C'est la notion que la première version de ce chapitre n'avait
pas, et son absence faisait écrire `SELECT <fenêtre>, <banque>`, qui suppose
chaque fenêtre choisie indépendamment. Le matériel refuse cette hypothèse :

- CPC, `ccc = 011` : le bloc 3 de la RAM de base passe en `&4000` **et** le bloc
  3 de la page étendue en `&C000`, d'un seul geste ;
- CPC, `ccc = 010` : les quatre fenêtres basculent ensemble ;
- ZX +2A/+3, mode spécial : exactement quatre combinaisons figées — `(0,1,2,3)`,
  `(4,5,6,7)`, `(4,5,6,3)`, `(4,7,6,3)`.

Aucune n'est décomposable en choix par fenêtre. À l'inverse, le PPI du MSX
*est* authentiquement indépendant : deux bits par page, produit cartésien
complet. Le modèle traite donc l'indépendance comme le **cas particulier** — un
produit que le profil décrit paramétriquement et n'écrit pas à la main — et non
comme la règle.

Un profil déclare un ou plusieurs **axes** de configuration (`CONFIG SET`).
L'état de la machine est le produit des axes ; chaque axe porte son propre
`SELECT`. Un axe peut en recouvrir un autre — sur CPC une ROM cache la RAM
**en lecture** seulement, l'écriture continuant d'atteindre la banque RAM — et
c'est le profil qui déclare cette priorité (`OVER`), parce que la lire à
l'envers ferait déclarer conforme un octet écrit dans le vide.

#### Sur quels états le linker raisonne

Le produit des axes est immense — sur CPC, huit configurations × huit pages ×
deux axes de ROM × 256 numéros de ROM — et le linker ne connaît **aucune
séquence d'exécution** : il ne sait pas sous quel état une routine tourne. Le
refus du §12.2 doit donc se formuler sans trace d'exécution :

> Un accès de la section `X` vers la section `Y` est licite s'il existe au moins
> un état où les banques de `X` et de `Y` sont simultanément visibles.

Cet énoncé ne s'évalue pas en énumérant les états : il se décide **axe par
axe** — deux banques sont co-visibles si aucun axe ne les met dans la même
fenêtre. Coût constant, même verdict, et aucune dépendance à ce que l'auteur a
pris la peine d'écrire dans son script : un script qui ne nomme rien vérifierait
sinon quelque chose de vide.

**`SELECT`** — comment on atteint une configuration. Deux choses, pas une :
**les nombres**, et **les contraintes vérifiables**.

Les nombres ne sont pas toujours une suite d'écritures constantes :

```
SELECT <axe> = OUT  <port>, <valeur>      // CPC, ZX
SELECT <axe> = POKE <adresse>, <valeur>   // mappers de mega-ROM MSX
```

Un modèle qui ne connaîtrait que `OUT` — le réflexe qu'on prend en ne regardant
que le CPC — ne saurait pas décrire un MSX, où la commutation d'une mega-ROM est
une **écriture mémoire**. Et le contraste se produit *au sein d'une même
machine* : le memory mapper RAM du MSX, lui, commute bien par port.

Deux formes de plus, que « une suite d'écritures » ne couvre pas :

- **le port peut être fonction de la banque.** Au-delà de 512 K de RAM CPC, une
  partie du numéro de banque est dans l'**adresse** du port (`&7Fxx`, `&7Exx`, …,
  bits A10-A8, généralement inversés) : ce n'est plus une constante.
- **un registre peut être subordonné.** `RMR2` du CPC Plus n'a d'effet que si un
  bit d'un autre registre est à 0, et exige au préalable une séquence de
  déverrouillage ASIC non documentée.

Et les contraintes, que le linker sait **vérifier** sans jamais écrire une
instruction de commutation :

| contrainte déclarée | ce qu'elle interdit | machine qui l'impose |
|---|---|---|
| fenêtre d'exécution interdite | commuter depuis un code qui vit dans une fenêtre que la commutation change (§13.3) | CPC, MSX |
| interruptions coupées | commuter les interruptions actives, le vecteur pouvant partir avec la banque | toutes |
| pile hors d'une plage | commuter alors que `SP` pointe dans ce qui va changer | CPC `ccc = 010`, modes *all-RAM* du +3 |
| plage commutant par accident | y placer une donnée : l'écrire commute | MSX Konami4, où **toute** écriture entre `&6000` et `&BFFF` commute |
| séquence imposée | commuter un sous-slot MSX en page 1 sans le faire passer par la page 3, depuis un code hors de `&C000`-`&FFFF`, pile comprise | MSX |

C'est le prolongement direct du §13.3 : « le linker vérifie l'identité et
l'accord structurel, il ne fait que consigner l'équivalence » se généralise en
**« le linker vérifie les contraintes de commutation ; il n'écrit jamais la
commutation. »** Embarquer un stub de commutation canonique par machine
reviendrait à générer du code de chargement, précisément la dérive que ce
document combat (§12.4).

#### Qui attrape une contrainte, et à quelle couche

Ces contraintes posent un problème de couches, non de détail. Le §1 pose que
l'assembleur ne connaît aucune machine : il ne peut donc refuser ni
`in a,(&7FFD)` sur un port en écriture seule, ni `ld (&7000),a` dans une plage
`CLOBBERS`. Et le linker, qui connaît le profil, ne voit plus que des octets — il
a perdu l'instruction. Sur Konami4, où `CLOBBERS` couvre 24 K sans une seule zone
de données sûre, un contrôle qui tomberait dans cet interstice ne vaudrait rien.

Le partage se fait donc en deux moitiés, et la seconde est ce qui rend le
contrôle réel plutôt que décoratif :

1. **Le placement** — aucune section de données dans une plage `CLOBBERS`.
   Décidable, immédiat, et déjà utile seul.
2. **Les accès à adresse littérale** — l'assembleur les consigne dans le
   quatrième bloc du fichier objet (§4.6) sans les interpréter ; le linker les
   confronte au profil. C'est le motif de la relocalisation, appliqué à autre
   chose : l'assembleur note, le linker tranche, et le §1 reste intact.

Ce que cela n'attrapera jamais : un `ld (hl),a` dont `HL` est calculé. La limite
est écrite ici pour être connue, non découverte.

#### Où s'accrochent les attributs

La première version de ce chapitre les mettait tous sur la fenêtre ou la banque,
indistinctement. Les faits imposent **trois porteurs** :

| attribut | porteur | pourquoi pas la fenêtre |
|---|---|---|
| `CONTENDED` | **banque** | ZX : la contention frappe certaines banques RAM (1,3,5,7 sur 128/+2 ; 4,5,6,7 sur +2A/+3). `&C000`-`&FFFF` est donc lent ou non **selon ce qui y est paginé** ; un attribut de fenêtre ne peut pas l'exprimer. |
| `ro` / `rw` matériel | **banque** | une ROM est en lecture seule où qu'elle apparaisse. |
| `VIDEO` | **banque** | CPC : la RAM étendue n'est jamais lue par le CRTC. Aucune section écran ne peut y vivre — un refus de placement que seul le linker peut prononcer. |
| lecture seule *effective* d'une fenêtre | **configuration** | +2A/+3 en mode spécial : `&0000`-`&3FFF` porte de la RAM. « La fenêtre ROM est `READONLY` » y est faux. Cela se **déduit** de la banque présente dans la configuration. |
| `LOCKS` | **pagination entière** | ZX : le bit 5 de `&7FFD` verrouille aussi `&1FFD`. Ce n'est pas l'attribut d'un port. |

Et un attribut disparaît : **`SHADOWS` ne se déclare plus, il se calcule.** Deux
configurations d'un même axe qui n'accordent pas la même banque à une fenêtre
disent, par leur seule existence, que passer de l'une à l'autre fait disparaître
ce qui était là. C'était la principale source d'erreur de saisie d'un profil, et
le piège du §12.2 devient une conséquence au lieu d'une déclaration. `ALWAYS`
suit le même sort : une fenêtre à laquelle toutes les configurations donnent la
même banque est fixe, et le linker le sait sans qu'on le lui dise.

### 13.2 Les trois profils, dans le même vocabulaire

Les valeurs citées ici sont celles de `docs/recherche/` — vérifiées sur sources
primaires, et signalées quand elles ne le sont pas. Les blocs restent des
esquisses de *forme* : c'est le vocabulaire qu'ils exposent au jugement, pas une
syntaxe arrêtée.

**Un seul profil est écrit : le CPC.** Le risque a changé de camp. Avant, le
vocabulaire était CPC et n'aurait pas survécu au deuxième profil ; maintenant, ce
chapitre en contient trois alors que la cible est le CPC seul, et le danger est
de payer la conception de trois machines pour n'en livrer aucune. Les blocs ZX et
MSX ci-dessous ne sont donc **pas une feuille de route** : leur fonction est de
tester le vocabulaire, et elle est déjà remplie — ce sont eux qui ont fait tomber
`SELECT <fenêtre>, <banque>`, `SHADOWS`, le 16 K câblé et le `CONTENDED` porté
par la fenêtre. Les sortir du document les rendrait décoratifs ; les prendre pour
un plan de travail coûterait deux machines que personne ne cible.

**CPC 6128 + extension** — c'est le bloc du §6 : quatre fenêtres de 16 K, huit
banques de 16 K par page de 64 K, huit configurations énumérées, un `SELECT` en
`OUT` sur le port du PAL avec la valeur `11pppccc`, et l'attribut `VIDEO` sur les
seules banques de base.

**ZX Spectrum 128 / +2** — la structure est proche du CPC, avec une nuance que la
première version de ce chapitre manquait : la contention est portée par les
banques.

```
TARGET zx128

WINDOW w0 [&0000..&3FFF]
WINDOW w1 [&4000..&7FFF]
WINDOW w2 [&8000..&BFFF]
WINDOW w3 [&C000..&FFFF]

BANK ram0..ram7  SIZE &4000  rw
BANK ram1, ram3, ram5, ram7   CONTENDED      // attribut de banque, pas de fenêtre
BANK ram5, ram7  VIDEO                       // écran normal / shadow
BANK rom0, rom1  SIZE &4000  ro

CONFIG SET pager {
    high<n> [CODE n] { w1 ram5  w2 ram2  w3 ram<n> }   // n = 0..7
}
SELECT pager = OUT &7FFD, __state | CODE     // bits 0-2 : banque en w3
                                             // bit 3 : écran affiché
                                             // bit 4 : numéro de ROM
                                             // bit 5 : verrou
PAGING LOCKS ON &7FFD BIT 5                  // porte sur la pagination entière
PAGING WRITE_ONLY                            // et une lecture n'est pas neutre
```

Trois choses que seul le profil peut porter. Le port est en **écriture seule** :
au source de tenir une copie de l'état — d'où le `__state` — et sur 128/+2 gris
précoces une lecture n'est pas neutre du tout, le décodage ne distinguant pas
lecture et écriture, ce qui plante typiquement la machine. `w1` est toujours la
banque 5, qui est contended : une section au timing critique n'a rien à y faire —
environ 25 % de débit en moins, mais **parce que la banque l'est**, pas la
fenêtre. Et le verrou, une fois posé, bloque toute la pagination jusqu'au reset.

**Un profil `zx128` ne décrit pas un +3.** Ce sont deux cibles distinctes : le
+2A/+3 ajoute le port `&1FFD`, déplace la contention sur les banques 4 à 7,
compose un numéro de ROM sur deux bits pris dans deux ports, et surtout ouvre un
mode spécial de quatre configurations *all-RAM* où `&0000` porte de la RAM. C'est
l'exemple type de ce que `CONFIG` sait dire et qu'un modèle par fenêtre ne sait
pas :

```
TARGET zx3

CONFIG SET pager {
    normal<n>  { w0 rom<r>  w1 ram5  w2 ram2  w3 ram<n> }
    special0   { w0 ram0    w1 ram1  w2 ram2  w3 ram3 }
    special1   { w0 ram4    w1 ram5  w2 ram6  w3 ram7 }
    special2   { w0 ram4    w1 ram5  w2 ram6  w3 ram3 }
    special3   { w0 ram4    w1 ram7  w2 ram6  w3 ram3 }
}
```

En `special0`, rien n'est contended ; en `special1`, tout l'est. Aucune
déclaration `CONTENDED` supplémentaire n'a été nécessaire pour le dire.

**MSX** — c'est ici que les deux exigences du §13.1 se paient : plusieurs grilles
de fenêtres, et des banques dont la taille n'est pas 16 K.

```
TARGET msx-konami4                    // « Konami sans SCC » : le SCC est une
                                      //   autre cible, ses registres diffèrent

// Grille 1 : les pages de slot, 16 K, tout l'espace adressable
WINDOW page0 [&0000..&3FFF]
WINDOW page1 [&4000..&7FFF]
WINDOW page2 [&8000..&BFFF]
WINDOW page3 [&C000..&FFFF]

// Grille 2 : les fenêtres du mapper, 8 K, superposées aux pages 1 et 2
WINDOW m0 [&4000..&5FFF]
WINDOW m1 [&6000..&7FFF]
WINDOW m2 [&8000..&9FFF]
WINDOW m3 [&A000..&BFFF]

BANK seg<n>  SIZE &2000  ro           // 8 K, et non 16 K

// Quatre axes authentiquement indépendants : deux bits par page
CONFIG SET slot<p> FOR page<p> { s0  s1  s2  s3 }        // p = 0..3
SELECT slot0..slot3 = OUT &A8, slot3<<6 | slot2<<4 | slot1<<2 | slot0

// Le mapper : trois fenêtres commutables, une figée
CONFIG SET mapper1 { seg<n> } SELECT = POKE &6000, n
CONFIG SET mapper2 { seg<n> } SELECT = POKE &8000, n
CONFIG SET mapper3 { seg<n> } SELECT = POKE &A000, n
CONFIG SET mapper0 { seg0 }                    // &4000-&5FFF : figée au segment 0

SELECT mapper1..mapper3 CLOBBERS [&6000..&BFFF]   // toute écriture y commute
```

Quatre faits que ce bloc porte et que la version précédente ratait. Les banques
font **8 K** — quatre fenêtres de mapper, pas deux de 16 K —, et leur grille ne
coïncide pas avec les pages de slot : deux découpages simultanément actifs.
`&4000`-`&5FFF` n'a **pas** de registre sur Konami4, elle est figée. Le mapper ne
couvre ni `page0` ni `page3`. Et le piège de l'écriture est bien plus large que
« l'adresse canonique » : **toute** écriture entre `&6000` et `&BFFF` commute,
soit 24 K sans une seule zone de données sûre — d'où `CLOBBERS`, qui est
exactement ce que le linker sait vérifier et que l'assembleur ne saura jamais.
L'étendue varie d'un mapper à l'autre (`&6000`-`&7FFF` seulement pour ASCII8,
deux plages de 2 K pour ASCII16, les 2 K bas de chaque bloc pour Konami5) : c'est
une plage **par registre**, pas un attribut global.

Un `SELECT` de slot MSX complet est par ailleurs une suite d'écritures de
natures différentes : le port `&A8` ne suffit pas pour un slot **étendu**, il
faut aussi écrire dans le registre mappé en `&FFFF` — celui du slot primaire
sélectionné en page 3, qui se relit **complémenté**. D'où les contraintes de
séquence du §13.1 plutôt qu'une liste d'écritures.

> **Ce qui reste non confirmé sur source primaire**, et qu'un profil doit citer
> comme tel : le décodage du port `&7FFD` sur +2A/+3 ; l'association port ↔ page
> du memory mapper RAM MSX ; et l'ensemble des mappers de mega-ROM, dont aucun
> n'a de spécification constructeur — leurs valeurs viennent d'openMSX et de
> reverse engineering concordants.

#### Les tests d'acceptation du modèle

« Décrire une machine ne doit pas demander de toucher au code du linker » est la
seule affirmation de ce chapitre qui puisse se vérifier. Trois formes, à
échelonner — et la première s'écrit avant d'avoir un linker, ce qui est
précisément l'intérêt :

**1. Aujourd'hui, par revue.** Le tableau du §7 se relit comme une liste de
contrôle : pour chaque nature d'information, le vocabulaire a-t-il un mot qui la
dise ? Le profil ZX 128 est le sujet de l'exercice, parce que ses valeurs sont
déjà vérifiées sur sources primaires : il ne coûte donc qu'une relecture, et il
échoue de façon visible — un mot manque, ou une valeur n'a pas de porteur.

| nature (§7) | le mot qui la porte | pour le ZX 128 |
|---|---|---|
| grille de fenêtres | `WINDOW` | quatre de 16 K |
| banques et leur taille | `BANK … SIZE` | huit de 16 K, plus les ROM |
| lecture seule matérielle | `BANK … ro` | `rom0`, `rom1` |
| accès ralenti | `BANK … CONTENDED` | banques 1, 3, 5, 7 |
| visibilité vidéo | `BANK … VIDEO` | banques 5 et 7 |
| états atteignables | `CONFIG SET` | huit, un par banque en `w3` |
| mécanisme de sélection | `SELECT … OUT` | `&7FFD` |
| port fonction de la banque | *non employé ici* | sans objet |
| préconditions, séquences | *non employé ici* | sans objet |
| contraintes vérifiables | `PAGING WRITE_ONLY` | et une lecture n'est pas neutre |
| irréversibilité | `PAGING LOCKS` | bit 5 |
| pile, vecteur | `STACK`, `INT_VECTOR` (script) | déclarés par le programme |

Deux lignes « sans objet » ne sont pas un échec : elles disent que le CPC exige
du vocabulaire que le ZX n'emploie pas, ce qui est la situation normale d'un
modèle générique. Un échec serait une ligne sans mot.

**2. À C1, en données.** Le profil ZX 128 versionné comme fichier de données que
la CI lit, sur le modèle des six exécutables de test existants : un
`profile_test` qui charge chaque profil livré et vérifie qu'il se lit — sans
qu'aucun code de linker connaisse son nom.

**3. À C1, mécaniquement.** Un invariant qui s'exprime en une commande :
**aucun nom de machine dans le code du linker.** `grep -ril
'cpc\|zx\|msx\|amstrad\|spectrum'` sur les sources du linker doit ne rien
rendre. Il est inscrit maintenant, et non après, parce qu'un invariant écrit
après le code est un invariant qu'on affaiblit pour le faire passer.

#### Le cas du ZX Next

Le Next n'est pas un ZX 128 : il a sa propre MMU, par tranches plus fines, **en
plus** d'une émulation de la pagination du 128 — donc deux axes de pagination
coexistants, ce que le modèle d'axes prévoit mais que rien n'a encore confronté.
Aucune de ses valeurs n'est vérifiée dans ce dépôt, et `docs/recherche/` ne le
couvre pas : **aucune n'entre dans un profil avant son propre dossier de
recherche**, par la règle du §7.

Une prédiction, consignée comme telle et non comme un acquis : si le modèle est
bon, un profil Next s'écrit en deux `CONFIG SET` — pagination héritée, MMU — sans
une ligne de code de linker. S'il faut davantage, c'est le modèle d'axes qui est
à revoir, et il vaut mieux l'apprendre sur une machine qu'on a sous la main.

### 13.3 La continuité à travers une commutation

Le recouvrement du §12.2 — du code à `&4000` qui disparaît quand on commute une
banque à `&4000` — n'est pas une bizarrerie CPC. Mais sa formulation évidente,
« une routine ne peut pas s'exécuter dans la fenêtre qu'elle commute », est
**fausse**, et le contre-exemple n'est pas exotique : sur une mega-ROM MSX, les
quatre fenêtres peuvent être des banques de ROM, il n'existe aucun résident où se
réfugier, et la solution universelle est de **répliquer le stub de commutation au
même offset dans toutes les banques**. Le PC continue alors sur les mêmes octets.
C'est la pratique standard sur toute architecture à cartouche.

Le CPC a le luxe d'une fenêtre toujours résidente ; prendre ce luxe pour une loi
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

Étant un placement, elle s'écrit dans le script et non dans la source, et c'est
la seule forme qui nomme plusieurs banques d'un coup :

```
CONFIG SET ram {
    MIRROR [ext0..ext3] AT OFFSET 0x0000 { SECTION switch_stub }
}
```

Le chiffrage que la sous-section « deux conséquences pratiques » exige est émis
d'office, sans qu'on le demande : `miroir : 20 octets × 4 banques = 80 octets`.

Une conséquence à ne pas rater, parce qu'elle touche le glossaire : le miroir est
le **seul** placement où un site d'émission produit N emplacements de rangement.
La *provenance* — l'attribution d'un octet écrit à une ligne de source déroulée —
doit donc supporter qu'une même ligne réponde de plusieurs octets à des
emplacements distincts. Le chevauchement, qui nomme aujourd'hui deux lignes en
conflit, doit savoir dire « ces deux octets viennent de la même ligne, dans deux
banques » sans le prendre pour une faute.

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

Le point important de la réponse est ce qu'il n'y a presque **rien** à faire.

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

#### Le seul ajout : une relation, `INIT_FROM`

Il manque néanmoins une chose, et ce n'est pas un quatrième type :

```
SECTION vars,     "rw"     INIT_FROM vars_image
SECTION vars_image, "ro"
```

« Cette section RAM est initialisée depuis cette section ROM. » Deux besoins
distincts s'y rejoignent : le couple `_INITIALIZED` / `_INITIALIZER` que `crt0`
recopie chez SDCC (§9), sans lequel la traduction d'un objet C perdrait
l'information ; et tout code CPC qui vit en ROM et tourne en RAM, que
`PHASE` / `DEPHASE` traite aujourd'hui à la main.

Une **relation** plutôt qu'un type, pour une raison mécanique : les trois types
décrivent la nature du contenu, la relation décrit le placement — et une
relation, contrairement à un type, est ce que le linker sait déjà vérifier. Deux
sections, même taille, l'une `"ro"` et l'autre `"rw"` ou `"uninit"` : le contrôle
s'écrit en une ligne, et il attrape la faute qui compte, celle où l'image et sa
destination ont divergé de taille après une modification.

La recopie, elle, reste du code de l'auteur (ou de son `crt0`). Le linker
fournit les nombres — adresse source, adresse destination, taille — et ne les
écrit pas, pour la même raison qu'au §12.4.

## Références

- Format CRO (Longshot / Logon System) —
  <https://github.com/Logon-System/CRO-Format/blob/main/README.md>
- Guide CRO —
  <https://github.com/Logon-System/CRO-Format/blob/main/doc/GuideCRO_ENG.md>
