# Le placement porté par le source

**Statut : plan, rien n'est engagé.**

## 1. Ce qu'on veut

Qu'une section dise **elle-même** où elle va, dans le vocabulaire du profil :

```asm
        section gfx1, "ro" IN ext_w1<1>
gfx1_data:
        db 0xA1, 0x10, 0x11, 0x12
```

Le linker place, et **fournit les valeurs de commutation** — le source n'écrit
plus aucun `equ` de port ni de valeur.

Trois conséquences qui font partie de la demande, et non de son interprétation :

- **`org` reprend son sens ordinaire** *à l'intérieur* d'une section : un
  décalage, pour une section qui ne commence pas à zéro. Il ne place plus.
- **`org b5:0x4000` n'est pas concerné.** C'est la notation héritée de
  l'ADR 0005, conservée pour les cas simples, sur le chemin d'émission directe.
  Elle **n'implique pas le linker**, et ce chantier ne la touche pas.
- **Le point de départ est le profil.** Ce que la section nomme est déclaré par
  le profil, pas inventé par le source.

## 2. Ce que c'est vraiment, dit sans détour

C'est le `MEMORY_MAP` du script, écrit sur la déclaration de section.

    CONFIG ext_w1<1> { w1 { SECTION gfx1 } }        ← le script, aujourd'hui
    section gfx1, "ro" IN ext_w1<1>                  ← la même chose, en source

**Ce n'est donc pas un second moteur de placement**, et c'est ce qui rend le
chantier abordable. Le chevauchement inter-sections (C1.5), le mou chiffré, le
découpage `[OFFSET, SIZE]` (C1.6), l'`ORG` déduit (C1.4), les symboles de
commutation (C1.7) : tout est construit, testé, et ne bouge pas. Ce qu'on ajoute
est une **seconde syntaxe d'entrée** vers la même carte.

## 3. Ce que la section nomme : la CONFIG, jamais la banque

Nommer la banque ne détermine pas de valeur de commutation. Sur `cpc6128`, la
banque `ext1` est amenée dans `w1` par **deux** états :

| état | CODE | valeur `ram` |
|---|---|---|
| `ext_w1<1>` | `%100 \| 1` = 5 | **&C5** |
| `all_ext`   | `%010` = 2      | **&C2** |

`IN ext1` vaudrait donc deux choses, et la règle en place — une seconde offre
d'un même nom avec une autre valeur **retire** le symbole (`link.cpp:1047`) —
ferait disparaître le symbole pour les quatre banques étendues, c'est-à-dire
pour l'exemple lui-même. `IN ext_w1<1>` en détermine une, et une seule.

## 4. La graphie *(tranchée)*

    section gfx1, "ro" IN ext_w1<1>          ; forme courte
    section gfx1, "ro" IN w1 OF ext_w1<1>    ; forme verbeuse, TOUJOURS valide

**La forme courte est licite quand la config ne mappe qu'une fenêtre** — elle
n'est alors pas ambiguë. Quand la config en mappe plusieurs, elle est **refusée
en nommant les fenêtres disponibles**, et la forme verbeuse est la réponse.

Ce refus est un diagnostic **du linker**, pas de l'assembleur : c'est le profil
qui dit combien de fenêtres une config mappe, et l'assembleur ne le lit pas (§5).

## 5. Pourquoi ça coûte moins cher qu'il n'y paraît

**L'assembleur n'a pas à comprendre `ext_w1<1>`.** Il porte la chaîne telle
quelle dans l'objet, et c'est le **linker** qui la résout contre le profil. Le §1
tient intact : l'assembleur reçoit un texte qu'il n'interprète pas, comme il
reçoit déjà des nombres par `switchSymbols`. `no_machine_names.sh` n'est pas
menacé non plus — il garde le code du linker, et le linker continue de ne rien
savoir d'aucune machine.

**Une section ainsi placée est simplement RELOCALISABLE.** Ses labels sont des
offsets résolus au linkage. C'est exactement pourquoi `org` y redevient un
décalage, et c'est la machinerie de l'étage B, déjà là.

**Le format objet est en clés** — `section "gfx1" id=… size=… max=… line=…`
(`fo.cpp:137`) — donc l'extension est additive.

## 6. Ce que ça rompt, et qu'il faut écrire

Le §11 dit : « Le source est agnostique. Aucun couplage au CPC dans le `.asm` :
il est réutilisable. » Écrire `IN ext_w1<1>` dans un `.asm` **rompt cela
délibérément**.

C'est défendable — l'en-tête d'`examples/aliased_org.asm` le dit déjà : *« un
placement peut être une propriété du PROGRAMME »* — mais ça demande **un ADR qui
l'énonce**, pas un silence. Il doit dire ce qu'un tel source perd : il ne se
recompile pas pour une autre machine sans être édité, là où `aliased.asm` change
de carte en changeant de script.

## 7. La surcharge *(tranchée)*

**Le script gagne, avec un avertissement.** Quand un script place une section que
le source place aussi, le placement du script s'applique et un avertissement dit
les deux. Motif : reprendre un source dont on ne veut pas éditer les sections est
un usage réel, et le refus l'interdirait. L'avertissement est ce qui empêche la
surcharge d'être silencieuse — le grief habituel contre deux porteurs pour une
décision.

## 8. Les étapes

| # | Étape | Bloqué par | État |
|---|-------|-----------|------|
| P1 | Les clés du profil seul dans `switchSymbols` | — | **faite** |
| P2 | La grammaire `IN`, portée opaquement jusqu'à l'objet | — | **faite** |
| P3 | Le linker résout le placement du source, et le fusionne | P2 | à faire |
| P4 | La surcharge par le script, et son avertissement | P3 | à faire |
| P5 | L'exemple et les tests d'acceptation | P1, P4 | à faire |
| P6 | L'ADR : le source qui porte son placement, et ce qu'il perd | tout | à faire |

### P1 — les clés du profil seul

`switchSymbols` (`link.cpp:1292`) boucle aujourd'hui sur `sc.map` : **sans
script, elle ne rend rien**, parce que ses clés sont des noms de section ou
d'état venus du script. Or elle est appelée **avant d'assembler**
(`asm_main.cpp:220`), donc elle ne peut pas savoir ce que le source nomme : les
clés doivent se dériver du **profil seul**.

**Clé = l'état, plus son argument s'il en a un.**

    __port_ram_ext_w1_1   __val_ram_ext_w1_1     ; état `ext_w1<1>`
    __port_ram_all_ext    __val_ram_all_ext      ; état sans paramètre
    __val_ram_linear

C'est la graphie « par état » de C1.7 prolongée, non une quatrième forme : la
forme sans argument est littéralement celle d'aujourd'hui, et l'argument dans le
nom est ce qui lève l'ambiguïté que C1.7 avait dû trancher par un retrait.

- [x] Sans script, avec le seul profil, `switchSymbols` rend les états du profil
- [x] `__val_ram_ext_w1_1` = `&C5` et `__val_ram_all_ext` = `&C2` : les deux
      cartes qui voient `ext1` sont **distinguées**, pas fusionnées
- [x] Un état sans paramètre garde sa graphie d'aujourd'hui
- [x] La valeur reste **bornée aux bits de l'axe** (D7) — l'invariant de C1.7 ne
      se perd pas en changeant de source de clés
- [x] `__mask_<axe>` sort aussi sans script
- [x] Aucun nom offert par ce chemin ne peut effacer un nom offert par le chemin
      script *(la fusion, ci-dessous)*
- [x] **Avec un script, rien ne change** : `accept_banked` et `accept_profile`
      rendent les mêmes octets

**Quatre défauts trouvés par la relecture, et corrigés** — tous du même genre :
le chemin du profil offrait des cartes que le matériel ne peut pas atteindre, et
se taisait sur celles qu'il peut.

- **L'énumération du paramètre est une INTERSECTION, non une réunion.** Avec deux
  fenêtres dont les familles de banques n'ont pas la même taille — `w0 lo<b>` sur
  `lo0..lo1` et `w1 hi<b>` sur `hi0..hi3` —, les valeurs 2 et 3 étaient offertes
  alors que `lo2` n'existe pas. Le placement refuse exactement cela (« the profile
  declares no such bank ») ; le symbole, lui, rendait un nombre vraisemblable
  pour une carte inatteignable.
- **Une banque qui ne résout pas interrompt le calcul.** `PAGE = bk ? page : 0`
  laissait `emit` aller au bout pour une fenêtre dont la banque n'existe pas.
  C'est ce qui transformait le défaut ci-dessus en symbole émis plutôt qu'en
  carte sautée.
- **Un état qui ne mappe AUCUNE fenêtre en est un quand même.** Le `off` d'un axe
  de recouvrement est ce qui **rend** la RAM — et c'est une valeur qu'aucun
  script ne pourra jamais offrir, puisqu'il n'y a rien à y placer. La boucle sur
  les slots tournait zéro fois : sur le profil livré, `__val_rom_lower_off`
  n'existait pas, et l'axe `rom_upper` n'offrait **rien du tout**, `__mask_rom_upper`
  compris — dont le §12.3 a besoin pour toucher un bit du port `&7F00` sans
  écraser les trois autres axes qui le partagent.
- **Un port ne survit pas à sa valeur.** Le port et le masque sont les mêmes par
  toutes les fenêtres d'un état là où la valeur peut différer : la règle de
  retrait n'effaçait que la valeur, et laissait la moitié d'un couple que le
  §12.3 emploie d'un bloc. Le masque, lui, reste — il appartient à l'**axe**, pas
  à l'état.

**Deux choses décidées en cours de route, à relire quand P6 s'écrira.**

- **La fusion est à SENS UNIQUE.** La graphie « par état » est offerte par les
  deux chemins, et ils ne répondent pas à la même question : le script la calcule
  **pour la fenêtre qu'il a placée**, le profil pour toutes les fenêtres de
  l'état. Un état dont deux fenêtres donnent deux valeurs aurait donc, par la
  règle de retrait, fait **disparaître** un symbole qui marchait. Le profil offre
  dans un second panier, et ne verse dans le premier que les noms qu'il ne
  contient pas — le script, plus spécifique, gagne, et un nom que le script a
  lui-même retiré reste retiré : ce retrait était une décision, pas un manque.
- **Un paramètre non borné n'offre rien.** Les valeurs énumérées sont celles que
  les banques **déclarées** bornent : `ext<b>` avec `BANK ext0..ext3` en donne
  quatre. `rom_hi<n>`, banque déclarée paramétriquement dont le numéro vient du
  matériel, n'en borne aucune — l'énumérer demanderait d'inventer une borne que
  le profil ne dit pas. `__val_rom_upper_on_15` n'existe donc pas sans script, et
  le diagnostic d'`EXTERN` non résolu le dit. C'est un manque **nommé**, dans le
  code et dans `docs/syntax.md`.

Vérifié de bout en bout sur le profil livré, sans script et sans placement :

        ld  bc, __port_ram_ext_w1_1 + __val_ram_ext_w1_1   →  01 C5 7F
        ld  bc, __port_ram_ext_w1_3 + __val_ram_ext_w1_3   →  01 C7 7F
        ld  bc, __port_ram_linear   + __val_ram_linear     →  01 C0 7F
        ld  bc, __port_ram_all_ext  + __val_ram_all_ext    →  01 C2 7F

### P2 — la grammaire `IN`

- [x] `section <nom>, "<type>"[, <max>] IN <config>` est analysé
- [x] `... IN <fenêtre> OF <config>` aussi
- [x] La chaîne est portée **sans être interprétée** : l'assembleur ne lit aucun
      profil
- [x] Une section ainsi déclarée est **relocalisable** ; `org` à l'intérieur est
      un décalage et ne place pas
- [x] L'objet la porte : deux clés additives dans l'enregistrement `section`
- [x] Aller-retour d'objet conservé (la propriété de `fo_test`)
- [x] `beautify` connaît la forme ; `docs/syntax.md` la décrit
- [x] Sans `IN`, une `section` est **exactement** ce qu'elle est aujourd'hui

**Trois choses décidées en cours de route.**

- **`IN` et `OF` ne sont PAS des mots réservés.** Ils ne valent que parmi les
  opérandes d'un `SECTION`, et un source qui nomme un label `in` continue
  d'assembler. Les réserver aurait cassé des sources existantes pour un mot qui
  n'est ambigu nulle part — `section in, "ro"` se distingue de `IN` par ce qui
  le suit, et la détection le voit.
- **Un `org` préfixé d'une banque dans une section placée est refusé.** `IN` a
  déjà dit où elle va ; le préfixe le redit, et rien ne garantirait qu'ils
  restent d'accord. Un `org` **nu**, lui, est licite et vaut un décalage : c'est
  ce que l'auteur écrit quand sa section ne commence pas à zéro.
- **Le placement est figé à la première déclaration**, comme le type et le
  plafond. Une réouverture muette le garde ; une réouverture qui le change est
  refusée. Le laisser bouger depuis un fichier inclus déplacerait la section
  sans un mot.

**Ce que P2 ne fait pas encore, et qu'il ne faut pas prendre pour un oubli :**
le linker **ignore** `place` pour l'instant. Une section déclarée `IN` est
relocalisable et se pose comme n'importe quelle section que personne n'a placée.
C'est P3, et c'est l'étape suivante — la syntaxe est acceptée et portée, elle
n'est pas encore honorée.

### P3 — le linker résout et fusionne

- [ ] Le placement du source entre dans la **même carte** que celle du script —
      un seul moteur, donc un seul jeu de diagnostics
- [ ] Une config que le profil ne déclare pas : refus, en nommant celles qu'il
      déclare
- [ ] Forme courte sur une config qui mappe plusieurs fenêtres : refus, en
      nommant les fenêtres et la forme verbeuse
- [ ] Une fenêtre que la config ne mappe pas : refus
- [ ] Le chevauchement, le mou et l'`ORG` déduit s'appliquent **sans une ligne
      de plus** — c'est le contrôle qui prouve que P3 n'a pas créé un second
      moteur
- [ ] Le type de section et les droits de la banque sont confrontés (une `"rw"`
      dans une banque `ro` est refusée), par le chemin qui le fait déjà
- [ ] **L'ordre de concaténation** de deux sections placées par le source dans
      le même bloc est celui de la fusion par nom de C1.0 — ordre des objets sur
      la ligne de commande, puis ordre de déclaration. À relire : il rend le
      placement dépendant de l'ordre de linkage, ce que le script ne fait pas

### P4 — la surcharge

- [ ] Le script gagne sur le source
- [ ] Un avertissement nomme les deux placements et la section
- [ ] Une section que seul le source place n'avertit de rien
- [ ] Un script qui place une section **inexistante** garde son diagnostic actuel

### P5 — l'exemple et les tests

`examples/aliased_sym.asm` : le même programme que `aliased.asm` et
`aliased_org.asm`, placé par ses **sections**, sans un seul `equ` — les cinq
valeurs viennent du linker.

    ld  bc, __port_ram_ext_w1_1 + __val_ram_ext_w1_1
    out (c), c

`tests/accept_aliased.sh` affirme aujourd'hui `aliased == aliased_org` octet pour
octet. Il affirmerait les **trois**. Aucun oracle extérieur : trois chemins du
même outil, et leur accord fait la preuve — le moule du dépôt.

- [ ] Les trois fichiers rendent les mêmes octets
- [ ] `aliased_sym.asm` ne contient ni `equ` de commutation, ni `org bN:`
- [ ] Déplacer une section d'une config à l'autre **dans le source seul** change
      sa banque et sa valeur, et pas une adresse logique — le contrôle de C1.9,
      transposé au source
- [ ] Le cas simple ne paie rien (D12) : sans `--target`, sans script et sans
      `IN`, les cinq exemples rendent les mêmes octets, `--sym` et diagnostics
      compris
- [ ] Les onze suites vertes, par les deux chaînes

### P6 — l'ADR

- [ ] Il énonce la rupture du §11, et ce qu'un tel source perd
- [ ] Il dit pourquoi la section nomme la **config** et non la banque (§3)
- [ ] Il dit pourquoi `org bN:` reste hors de ce dessin, et n'est pas déprécié
- [ ] L'ADR 0005 et l'ADR 0026 sont relus à sa lumière

## 9. Ce que ce chantier ne livre PAS

**Les clés par SECTION restent script-seul.** `__val_ram_gfx1` demande de savoir
quelle config voit `gfx1`. Avec `IN`, le source le dit — mais `switchSymbols`
tourne **avant** l'assemblage et ne peut pas le lire. Le linker le pourrait sur
son chemin `EXTERN` (`link.cpp:496`), au prix d'une seconde définition de la même
clé selon la provenance. À écarter explicitement, ou à traiter à part.

## 10. Où ranger le chantier *(tranché)*

**Chantier autonome.** L'étage C1 est clos par C1.10 et ceci n'est pas C2 : ça ne
vérifie rien, ça place et calcule. Rouvrir C1 d'un `C1.11` réécrirait une clôture
qui a une valeur — et ce chantier rompt le §11, ce qu'aucune étape de C1 n'a
fait.
