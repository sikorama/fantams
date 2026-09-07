# Le format objet `.rel` (ASxxxx / sdas–sdld) et les conventions SDCC/Z80

Recherche documentaire destinée à confirmer ou réfuter les affirmations de la
sous-section « Interopérabilité avec SDCC » de `docs/spec-chaine-outils.md`
(encadré « À vérifier avant de s'engager »).

Chaque affirmation ci-dessous porte sa source. Ce qui n'a pas été trouvé est
écrit comme non trouvé.

---

## 0. Versions consultées

| Objet | Version | Provenance |
|---|---|---|
| SDCC (source complet) | **4.6.0**, publiée le 2026-06-22 | `sdcc-src-4.6.0.tar.bz2`, https://sourceforge.net/projects/sdcc/files/sdcc/ ; `.version` du tarball contient `4.6.0` |
| SDCC (contre-vérification) | **4.5.0**, publiée le 2025-01-28 | `sdcc-src-4.5.0.tar.bz2`, même origine |
| Documentation ASxxxx incluse dans SDCC | **Version 5.05, August 2012** | `sdas/doc/asmlnk.txt`, ligne 621 : `ASxxxx Cross Assemblers, Version 5.05, August 2012` — identique dans 4.5.0 et 4.6.0 |
| Documentation ASxxxx amont (site d'Alan Baldwin) | **Version 6.10, July 2026** ; page linker « Last Updated: May 2026 » | https://shop-pdp.net/ashtml/asxdoc.htm et https://shop-pdp.net/ashtml/aslink.htm |
| Linker SDCC (`sdld`) | chaîne de version interne `V05.50.4-SDLD`, copyright 2025 | `sdas/linksrc/aslink.h:43` (SDCC 4.6.0). Dans 4.5.0 la même macro valait `"V03.00/V05.40 + sdld"` : **la base amont a été resynchronisée entre 4.5.0 et 4.6.0.** |

Toutes les références « fichier:ligne » ci-dessous portent sur l'arborescence
**SDCC 4.6.0** sauf mention contraire. Les divergences constatées avec 4.5.0
sont signalées.

Note importante sur le versionnement : SDCC n'utilise pas ASxxxx tel quel mais
un fork, `sdas` (assembleurs) et `sdld` (linker). Le manuel SDCC l'écrit :
« SDCC uses a modified version of ASXXXX » (`doc/sdccman.lyx:352`, note de bas de
page nommant `sdas (sdasgb, sdas6808, sdas8051, sdasz80)`), et plus loin :
« SDCC used an about 1998 branch of asxxxx version 2.0 which unfortunately is not
compatible with the more advanced […] ASxxxx Cross Assemblers nowadays available
from Alan Baldwin […]. In 2009 Alan made his ASxxxx Cross Assemblers version 5.0
available under the GPL license […] so a reunion is now a work in progress. »
(`doc/sdccman.lyx:9765–9779`).

---

## 1. Classement des sources

### Sources primaires utilisées

1. **Documentation ASxxxx officielle d'Alan Baldwin**, en ligne :
   - index : https://shop-pdp.net/ashtml/asxdoc.htm
   - chapitre linker : https://shop-pdp.net/ashtml/aslink.htm
   - corps du chapitre linker (formats d'entrée) : https://shop-pdp.net/ashtml/asls01.htm
     — ancres `#Input3` (« Linker V3 Input Format ») et `#Input6`
     (« Linker V6 (5 and 4) Input Format »).
   - texte intégral V6.00 : https://shop-pdp.net/ashtml/asmlnk.txt (et `.pdf`, `.rtf`)
2. **Copie de la documentation ASxxxx V5.05 livrée dans SDCC** :
   `sdas/doc/asmlnk.txt` (section 1.4.22 `.area Directive`, section 3.6
   « ASXXXX VERSION 3.XX LINKING »).
3. **Documentation du format propre à SDCC** : `sdas/doc/format.txt` (142 lignes),
   qui est la description du format objet **telle que sdas l'écrit réellement**,
   escapes compris.
4. **Code source sdas / sdld** : `sdas/asxxsrc/asout.c`, `sdas/asz80/z80pst.c`,
   `sdas/asz80/z80mch.c`, `sdas/linksrc/{aslink.h,lkmain.c,lkarea.c,lkhead.c,lkrloc.c,lksym.c,lkbank.c}`.
5. **Manuel SDCC officiel** : `doc/sdccman.lyx` (source LyX du manuel livré avec
   le compilateur ; la version en ligne est https://sdcc.sourceforge.net/doc/sdccman.pdf,
   non consultée — les citations proviennent du source livré, plus fiable pour
   le numéro de version).
6. **Code source du compilateur SDCC** : `src/z80/main.c`, `src/z80/mappings.i`,
   `src/z80/gen.c`, `src/SDCCasm.c`, `src/SDCCglue.c`, `src/SDCCmem.c`,
   `src/port.h`, `device/lib/z80/crt0.s`.

### Sources secondaires

**Aucune source secondaire n'a été utilisée.** Aucun wiki, forum ou billet de
blog n'a servi à étayer une affirmation de ce document. Les points restés sans
réponse (section 9) le sont restés plutôt que d'être comblés par une source
secondaire.

---

## 2. A1 — Où se trouve la spécification faisant autorité ?

Il y a **deux spécifications distinctes**, et c'est le premier fait à retenir.

### 2.1 La spécification ASxxxx amont (Alan Baldwin)

Le chapitre linker documente **quatre formats d'entrée** et non un seul :

> « ASLINK is the companion linker for the ASxxxx assemblers. The linker supports
> versions 3, 4, 5 and 6 of the ASxxxx assemblers. Object files from version 3, 4,
> 5, and 6 may be freely mixed while linking. Note that version 3 object files
> contain only a subset of the options available in versions 4, 5, and 6. Only
> version 6 supports complex relocations. »
> — https://shop-pdp.net/ashtml/aslink.htm

Les deux formats sont décrits dans la même page :

- **« Linker V3 Input Format »** — https://shop-pdp.net/ashtml/asls01.htm#Input3
- **« Linker V6 (5 and 4) Input Format »** — https://shop-pdp.net/ashtml/asls01.htm#Input6

Dans la copie V5.05 livrée avec SDCC, le format V3 est la section
**3.6 « ASXXXX VERSION 3.XX LINKING »**, sous-sections 3.6.1 à 3.6.10
(`sdas/doc/asmlnk.txt`, table des matières lignes 148–158, corps à partir de la
ligne 5132).

### 2.2 La spécification du dialecte sdas/sdld

`sdas/doc/format.txt` décrit le format **tel que sdas l'émet**, numéroté
2.5.1 à 2.5.8. C'est du V3 amont **augmenté** (voir §5.3 et §3.3). C'est la
source à privilégier pour écrire un lecteur, parce que c'est celle qui décrit
le mécanisme d'échappement que sdas utilise réellement.

### 2.3 Ce que sdld accepte en pratique

`sdld` **ne lit que le format V3**. Le numéro de version est câblé :

```c
/* sdas/linksrc/lkmain.c:593-596 */
        case 'X':
        case 'D':
        case 'Q':
                ASxxxx_VERSION = 3;
```

et le dispatch de relocalisation ne connaît que la version 3, le cas 4 étant
commenté :

```c
/* sdas/linksrc/lkrloc.c:81-90 */
	switch(ASxxxx_VERSION) {
	case 3:
		reloc3(c);
		break;
//      case 4:
//              reloc4(c);
//              break;
	default:
		fprintf(stderr, "Internal Version Error");
```

Il n'existe aucun autre point d'affectation de `ASxxxx_VERSION` dans
`sdas/linksrc/` (vérifié par recherche sur l'ensemble de `*.c` et `*.h`).

**Conclusion A1.** La spécification faisant autorité pour un lecteur de `.rel`
produits par SDCC/Z80 est, dans cet ordre :
`sdas/doc/format.txt` (dialecte réel) > `asls01.htm#Input3` (base amont) >
le code de `sdas/asxxsrc/asout.c` (l'écrivain) et `sdas/linksrc/lkrloc3.c`
(le lecteur) pour les cas non documentés.

---

## 3. A2 — Textuel ou binaire ? Types d'enregistrements

### 3.1 Textuel, ASCII, orienté ligne

> « The linkers' input object file is an ascii file containing the information
> needed by the linker to bind multiple object modules into a complete loadable
> memory image. »
> — https://shop-pdp.net/ashtml/asls01.htm#Input3

Le même énoncé figure dans le commentaire d'en-tête de `sdas/asxxsrc/asout.c`
(« The assemblers' output object file is an ascii file… »).

Tous les octets sont écrits en clair, un par un, séparés par une espace, dans le
radix courant :

```c
/* sdas/asxxsrc/asout.c, fonction out() */
	while (n--) {
		if (xflag == 0) {
			fprintf(ofp, " %02X", (*p++)&0377);
		} else
		if (xflag == 1) {
			fprintf(ofp, " %03o", (*p++)&0377);
		} else
		if (xflag == 2) {
			fprintf(ofp, " %03u", (*p++)&0377);
		}
	}
```

### 3.2 La première ligne : `[XDQ][HL][234]`

> ```
>         [XDQ][HL][234]
>                 X       Hexidecimal radix
>                 D       Decimal radix
>                 Q       Octal radix
>
>                 H       Most significant byte first
>                 L       Least significant byte first
>
>                 2       16-Bit Addressing
>                 3       24-Bit Addressing
>                 4       32-Bit Addressing
> ```
> — https://shop-pdp.net/ashtml/asls01.htm#Input3

Émission (`sdas/asxxsrc/asout.c:1063`, fonction `outradix`) :

```c
	if (xflag == 0) {
		fprintf(ofp, "X%c%d\n", (int) hilo ? 'H' : 'L', a_bytes);
	}
```

**Pour `sdasz80` concrètement : `XL4`.**
- `hilo = 0` (little-endian) et `exprmasks(4)` (adresses 32 bits) sont posés dans
  `minit()` : `sdas/asz80/z80mch.c:2136-2146`.
- `xflag == 0` (hexadécimal) est le défaut.

Ce `4` est récent. Le manuel SDCC le dit dans son historique de changements :

> « In 4.4.1, the address width in .rel files was increased from 24 bits to
> 32 bits for the z80 (and related) ports. »
> — `doc/sdccman.lyx:3114-3115`

Conséquence pratique : dans un `.rel` produit par `sdasz80` ≥ 4.4.1, les valeurs
de symbole font **8 chiffres hexadécimaux** et l'offset des lignes T fait
**4 octets**. Un lecteur qui suppose 16 bits lira faux.

### 3.3 Liste des enregistrements

Format V3 amont (https://shop-pdp.net/ashtml/asls01.htm#Input3) :

| Lettre | Rôle |
|---|---|
| `H` | Header |
| `M` | Module |
| `A` | Area |
| `S` | Symbol |
| `T` | Object code |
| `R` | Relocation information |
| `P` | Paging information |

Format V6 amont (`#Input6`) — ajoute `G` (Merge Mode) et `B` (Bank).

Ce que **sdld accepte réellement**, d'après le `switch` de `link_main()`
(`sdas/linksrc/lkmain.c:565-725`) :

| Lettre | Ligne | Traitement |
|---|---|---|
| `X` / `D` / `Q` | 593–595 | ligne de format ; fixe radix, `hilo`, `a_bytes`, et `ASxxxx_VERSION = 3` |
| `O` | 573 | **extension sdcc** : options du compilateur |
| `H` | 676 | header, `newhead()` |
| `M` | 691 | module, `module()` |
| `A` | 696 | area, `newarea()` |
| `S` | 706 | symbole, `newsym()` |
| `T` / `R` / `P` | 711–713 | `reloc(c)` |
| `;` | 720 | commentaire / magie NoICE (si compilé avec `NOICE`) |
| autre | `default` | **ignoré silencieusement** |

`G` et `B` ne sont **pas** dans ce `switch`. `newbank()` (`sdas/linksrc/lkbank.c:101`)
n'est appelé de nulle part dans `sdas/linksrc/*.c` (vérifié). Voir §7.

### 3.4 Sémantique, ligne par ligne

Les citations suivantes sont extraites de `sdas/doc/format.txt` (source
primaire SDCC), qui reprend le texte amont en le complétant.

**Header — `H aa areas gg global symbols`** (`format.txt:9-15`)

> « The header line specifies the number of areas(aa) and the number of global
> symbols(gg) defined or referenced in this object module segment. »

Émission : `fprintf(ofp, "H %X areas %X global symbols\n", narea, nglob);`
(`sdas/asxxsrc/asout.c:1158`).

Le parseur de sdld (`sdas/linksrc/lkhead.c:118-155`) lit en réalité une suite de
paires `<nombre> <mot-clé>` et reconnaît `areas`, `global`, **`banks`** et
**`modes`** — ces deux derniers étant du V4/V6 et n'étant jamais émis par sdas.
La comparaison des mots-clés est insensible à la casse (`symeq(..., 1)`).

**Module — `M name`** (`format.txt:18-24`)

> « The module line specifies the module name from which this header segment was
> assembled. The module line will not appear if the .module directive was not
> used in the source program. »

**Options SDCC — `O <texte>`** — *extension sdas, absente de la spec amont*

```c
/* sdas/asxxsrc/asout.c:1180 */
        if (is_sdas() && NULL != optsdcc) fprintf(ofp, "O %s\n", optsdcc);
```

Le texte est celui de la directive `.optsdcc` de la source assembleur
(`sdas/asxxsrc/asmain.c`, traitement de `.optsdcc`). SDCC l'émet en tête de
chaque fichier `.asm` :

```c
/* src/z80/main.c:1074-1080, _z80_genAssemblerStart() */
      tfprintf (of, "\t!optsdcc -m%s", port->target);
      fprintf (of, " sdcccall(%d)", options.sdcccall);
```

soit, pour le Z80 en réglage par défaut : `.optsdcc -mz80 sdcccall(1)`.
`sdld` compare cette chaîne entre modules et **avertit en cas de divergence** :

```c
/* sdas/linksrc/lkmain.c:573-590 */
                                fprintf(stderr,
                                        "?ASlink-Warning-Conflicting sdcc options:\n"
                                        "   \"%s\" in module \"%s\" and\n"
                                        "   \"%s\" in module \"%s\".\n", ...
```

C'est le seul endroit du format où l'ABI est déclarée. Un linker tiers a intérêt
à lire cette ligne : c'est ainsi qu'il sait si les modules ont été compilés en
`sdcccall(0)` ou `sdcccall(1)`.

**T — `T xx xx nn nn nn nn nn ...`** (`format.txt:58-65`)

> « The T line contains the assembled code output by the assembler with xx xx
> being the offset address from the current area base address and nn being the
> assembled instructions and data in byte format. »

L'offset fait `a_bytes` octets (donc **4** pour sdasz80) :
`out_txb(a_bytes, dot.s_addr)` dans `outchk()`, `sdas/asxxsrc/asout.c`.
Le texte amont V6 le confirme explicitement : « xx xx and nn nn can be 2, 3, or
4 bytes as specified by the .REL file header » (asls01.htm#Input6, T Line), et la
section « 24-Bit and 32-Bit Addressing » (asls01.htm#Input3) donne les formes
étendues des lignes S et T.

**P — `P 0 0 nn nn n1 n2 xx xx`** (`format.txt:123-142`)

> « The P line provides the paging information to the linker as specified by a
> .setdp directive. The format of the relocation information is identical to that
> of the R line. »

Sans objet pour le Z80 : la directive `.setdp` n'est pas dans la table de
`sdasz80` (`sdas/asz80/z80pst.c`, recherche de `setdp` : aucun résultat).
Un lecteur peut donc traiter `P` comme une erreur pour les objets Z80 — mais
prudemment, il vaut mieux l'ignorer.

---

## 4. A3 — Déclaration d'une area et liste complète des attributs

### 4.1 La ligne A

Format V3 (`sdas/doc/format.txt:43-56`, texte identique à
https://shop-pdp.net/ashtml/asls01.htm#Input3) :

> ```
>                 A label size ss flags ff
> ```
> « The area line defines the area label, the size (ss) of the area in bytes, and
> the area flags (ff). The area flags specify the ABS, REL, CON, OVR, and PAG
> parameters:
>
>                 OVR/CON  (0x04/0x00 i.e.  bit position 2)
>
>                 ABS/REL  (0x08/0x00 i.e.  bit position 3)
>
>                 PAG      (0x10 i.e.  bit position 4) »

**sdas ajoute un champ `addr` non documenté dans `format.txt`** :

```c
/* sdas/asxxsrc/asout.c:1349-1375, outarea() */
	fprintf(ofp, "A ");
	fprintf(ofp, "%s", &ap->a_id[0]);
	... frmt = " size %X flags %X";
        fprintf(ofp, frmt, ap->a_size, ap->a_flag);
        if (is_sdas()) {
                /* sdas specific */
		if (xflag == 0) {
                        fprintf(ofp, " addr %X", ap->a_addr);
		} ...
```

La ligne réellement écrite est donc :
`A _CODE size <ss> flags <ff> addr <aa>`.

Et **pour la cible Z80, `sdld` ignore ce champ `addr`** :

```c
/* sdas/linksrc/lkarea.c:156-165, newarea() */
        if (is_sdld() && !(TARGET_IS_Z80 || TARGET_IS_Z180 || TARGET_IS_GB)) {
                /*
                 * Evaluate area address
                 */
                skip(-1);
                axp->a_addr = eval();
        }
```

Un lecteur tiers doit donc **tolérer le champ `addr`** (sinon il casse sur tous
les `.rel` de sdas) et décider lui-même s'il en tient compte ; sdldz80 non.

La ligne A du format **V6** amont est `A label size ss flags ff [bank bb] [bndry mm]`
(https://shop-pdp.net/ashtml/asls01.htm#Input6, « Area Line ») — mais rien de
cela n'est émis par sdas.

### 4.2 Attributs reconnus par `sdasz80` à la directive `.area`

La table des mots-clés d'attribut de l'assembleur Z80 est exhaustive et courte :

```c
/* sdas/asz80/z80pst.c:72-77 */
    {   NULL,   "CON",          S_ATYP,         0,      A_CON   },
    {   NULL,   "OVR",          S_ATYP,         0,      A_OVR   },
    {   NULL,   "REL",          S_ATYP,         0,      A_REL   },
    {   NULL,   "ABS",          S_ATYP,         0,      A_ABS   },
    {   NULL,   "NOPAG",        S_ATYP,         0,      A_NOPAG },
    {   NULL,   "PAG",          S_ATYP,         0,      A_PAG   },
```

Sémantique (`sdas/doc/asmlnk.txt`, section 1.4.22 `.area Directive`, lignes
2017–2107, texte identique amont) :

| Attribut | Signification citée |
|---|---|
| `ABS` | « absolute (automatically invokes OVR) » |
| `REL` | « relocatable » |
| `OVR` | « overlay » — sections de même nom démarrent à la même adresse |
| `CON` | « concatenate » — sections de même nom sont appendues |
| `NOPAG` | « non-paged area » |
| `PAG` | « paged area » — « The section must be on a 256 byte boundary and its length is checked by the linker to be no larger than 256 bytes. » |

Et, du même passage :

> « The default area type is REL|CON; i.e. a relocatable section which is
> concatenated with other sections of code with the same area name. »
> « (CON not allowed with ABS) »
> « Multiple invocations of the .area directive with the same name must specify
> the same options or leave the options field blank, this defaults to the
> previously specified options for this program area. »
> « The name may be from 1 to 79 characters in length. »
> « **The .area names and options are never case sensitive.** »

Ce dernier point est vérifié dans le linker : la recherche d'area utilise
`symeq(id, ap->a_id, 1)` — le `1` étant le drapeau « Case Insensitive Compare »
(`sdas/linksrc/lkarea.c:223` et `sdas/linksrc/lksym.c:418-435`).

### 4.3 Valeurs numériques des bits de `flags`

Constantes du format V3, telles que le linker les définit
(`sdas/linksrc/aslink.h:265-281`) :

```
 * Area flags
 *
 *         7     6     5     4     3     2     1     0
 *      +-----+-----+-----+-----+-----+-----+-----+-----+
 *      |     |     |     | PAG | ABS | OVR |     |     |
 *      +-----+-----+-----+-----+-----+-----+-----+-----+

#define	A3_CON		000		/* concatenate */
#define	A3_OVR		004		/* overlay */
#define	A3_REL		000		/* relocatable */
#define	A3_ABS		010		/* absolute */
#define	A3_NOPAG	000		/* non-paged */
#define	A3_PAG		020		/* paged */
```

Extensions sdld, **pour d'autres cibles que le Z80**
(`sdas/linksrc/aslink.h:283-292`, encadrées par les commentaires
`/* sdld specific */` … `/* end sdld specific */`) :

```
/* Additional flags for 8051 address spaces */
#define A_DATA    0000          /* data space (default)*/
#define A_CODE    0040          /* code space */
#define A_XDATA   0100          /* external data space */
#define A_BIT     0200          /* bit addressable space */

/* Additional flags for hc08 */
#define A_NOLOAD  0400          /* nonloadable */
#define A_LOAD    0000          /* loadable (default) */
```

Ces mots-clés existent dans `sdas/as8051/i51pst.c:51-54` (`CODE`, `DATA`,
`XDATA`, `BIT`) mais **pas** dans `sdas/asz80/z80pst.c`. Leur origine est
documentée dans `sdas/doc/README` (John Hartman, 2-Apr-1998) :

> « I added four attributes to the .area directive to support the 8051's multiple
> address spaces: CODE for codespace, DATA for internal data, BIT for internal
> bit-addressable, XDATA for external data. »

**Réponse A3.** Pour un objet Z80 produit par `sdasz80`, la liste complète des
attributs d'area est : **`ABS`/`REL`, `OVR`/`CON`, `PAG`/`NOPAG`, et rien
d'autre.** Trois bits utiles dans `flags` (positions 2, 3, 4). Il n'y a ni
attribut de type mémoire, ni attribut de bank, ni alignement, ni `NOLOAD`.

### 4.4 Symboles `s_<area>` et `l_<area>`

La documentation amont attribue ces symboles à l'assembleur :

> « The ASxxxx assemblers also automatically generate two symbols for each program
> area: `s_<area>` This is the starting address of the program area. `l_<area>`
> This is the length of the program area. »
> — `sdas/doc/asmlnk.txt`, section 1.4.22

**Dans SDCC c'est le linker qui les crée, pas l'assembleur.** Aucune génération
de ces noms n'existe dans `sdas/asxxsrc/`, tandis que `sdld` les fabrique :

```c
/* sdas/linksrc/lkarea.c:467-491 */
                /*
                 * Create symbols called:
                 *      s_<areaname>    the start address of the area
                 *      l_<areaname>    the length of the area
                 */
                if (! symeq(ap->a_id, _abs_, 1)) {
                        strcpy(temp+2, ap->a_id);
                        *(temp+1) = '_';
                        *temp = 's';
                        sp = lkpsym(temp, 1);
                        sp->s_addr = ap->a_addr;
                        ...
                        *temp = 'l';
                        sp = lkpsym(temp, 1);
                        sp->s_addr = ap->a_size;
```

Comme les areas SDCC/Z80 s'appellent déjà `_DATA` (§8), les symboles produits
sont `s__DATA` et `l__DATA` — **avec deux underscores**. C'est exactement ce que
consomme le `crt0.s` de SDCC :

```asm
;; device/lib/z80/crt0.s, area _GSINIT
        ld      bc, #l__DATA
        ...
        ld      hl, #s__DATA
        ...
	ld	bc, #l__INITIALIZER
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
```

**Un linker qui prétend remplacer `sdld` doit définir ces symboles**, faute de
quoi `crt0.rel` et une partie de `device/lib` ne se lient pas. Ce point n'est
pas mentionné dans `docs/spec-chaine-outils.md`.

---

## 5. A4 — Encodage des relocalisations (lignes R / T)

### 5.1 Structure de la ligne R

`sdas/doc/format.txt:68-121` (texte SDCC, plus complet que l'amont) :

> ```
>                 R 0 0 nn nn n1 [n1x]  n2 xx xx ...
> ```
> « The R line provides the relocation information to the linker. The nn nn value
> is the current area index, i.e. which area the current values were assembled.
> Relocation information is encoded in groups of 4 (possibly 5) bytes:
>
>   1. n1 (and optionally n1x) is the relocation mode and object format:
>      1. bit 0 word(0x00)/byte(0x01)
>      2. bit 1 relocatable area(0x00)/symbol(0x02)
>      3. bit 2 normal(0x00)/PC relative(0x04) relocation
>      4. bit 3  1-byte(0x00)/2-byte(0x08) object format for byte data
>      5. bit 4 signed(0x00)/unsigned(0x10) byte data
>      6. bit 5 normal(0x00)/page '0'(0x20) reference
>      7. bit 6 normal(0x00)/page 'nnn'(0x40) reference
>      8. bit 7  LSB byte(0x00)/MSB byte(0x80) with 2-byte mode
>      9. bit 8  1 or 2 (0x00)/3-byte (0x100) object format for byte data.
>     10. bit 9  LSB or MSB (middle byte) (0x00) or byte 3 (real MSB) (0x200)
>         for 3-byte mode.
>
>   2. n2 is a byte index into the corresponding (i.e. preceeding) T line data
>      (i.e. a pointer to the data to be updated by the relocation). The T line
>      data may be 1-byte or 2-byte byte data format or 2-byte word format.
>
>   3. xx xx is the area/symbol index for the area/symbol being referenced. the
>      corresponding area/symbol is found in the header area/symbol lists. »

Les huit premiers bits sont **identiques** dans la spec amont V3
(https://shop-pdp.net/ashtml/asls01.htm#Input3, « R Line »), qui renvoie pour le
reste au code : « for the adhoc extension modes refer to asxxxx.h or aslink.h ».

Attention : le format **V6** amont réattribue complètement ces bits (bits <1:0>
= nombre d'octets, bit 6 = PC-relative, bit 7 = symbole ; `n2` porte en plus un
index de merge mode sur ses bits hauts) — voir asls01.htm#Input6. **Ne pas
confondre les deux tables de bits.** sdas/sdld n'utilisent que la table V3.

### 5.2 Le mécanisme d'échappement (spécifique sdas)

`sdas/doc/format.txt:94-101` :

> « If the upper four bits of n1 are set (i.e. (n1 & 0xf0) == 0xf0), it is taken
> as an escape character, and the relocation mode will consist of the lower four
> bits of n1 left shifted 8 bits or'ed with the value of n1x. If the upper four
> bits of n1 are not all set, then it is not an escape character, and the n1x byte
> is not present.
>
> This escape mechanism allows a 12-bit relocation mode value. »

Implémentation, côté écriture :

```c
/* sdas/asxxsrc/asout.c:325-352, write_rmode() */
    if ((r > 0xff) || ((r & R_ESCAPE_MASK) == R_ESCAPE_MASK))
    {
        if (r > 0xfff) { ... "relocation mode 0x%X too big." ... }
        *relp++ = R_ESCAPE_MASK | (r >> 8);
        *relp++ = r & 0xff;
    }
    else
    {
        *relp++ = r;
    }
    *relp++ = n;
```

avec `#define R_ESCAPE_MASK 0xf0` (`sdas/linksrc/aslink.h:378`, dans le bloc
`/* sdld specific */`). **Ce mécanisme n'existe pas dans la spec amont** : un
lecteur écrit d'après `asls01.htm` seul lira faux dès qu'un mode ≥ 0x100 ou de
forme `0xfX` apparaît.

### 5.3 Constantes de mode, telles que le code les définit

`sdas/linksrc/aslink.h:295-382` :

```
 * Relocation types.
 *
 *	       7     6     5     4     3     2     1     0
 *	    +-----+-----+-----+-----+-----+-----+-----+-----+
 *	    | MSB | PAGn| PAG0| USGN| BYT2| PCR | SYM | BYT |
 *	    +-----+-----+-----+-----+-----+-----+-----+-----+

#define	R3_WORD		0000		/* 16 bit */
#define	R3_BYTE		0001		/*  8 bit */
#define	R3_AREA		0000		/* Base type */
#define	R3_SYM		0002
#define	R3_NORM		0000		/* PC adjust */
#define	R3_PCR		0004
#define	R3_BYT1		0000		/* Byte count for R_BYTE = 1 */
#define	R3_BYTX		0010		/* Byte count for R_BYTE = X */
#define	R3_SGND		0000		/* Signed value */
#define	R3_USGN		0020		/* Unsigned value */
#define	R3_NOPAG	0000		/* Page Mode */
#define	R3_PAG0		0040		/* Page '0' */
#define	R3_PAG		0100		/* Page 'nnn' */
#define	R3_LSB		0000		/* output low byte */
#define	R3_MSB		0200		/* output high byte */
```

Modes « adhoc » réservés à d'autres cibles (mêmes fichier, lignes 363–371) :
`R3_J11` (saut 11 bits 8051), `R3_J19` (19 bits DS80C390), `R_C24` (24 bits) —
tous construits sur la combinaison « illégale » `R3_WORD | R3_BYTX`, avec la
mise en garde du fichier :

> « Additional "R3_" functionality is required to support some microprocesssor
> architectures. The 'illegal' "R3_" mode of R3_WORD | R3_BYTX is used as a
> designator of the extended R3_ modes. The extended modes replace the PAGING
> modes and are being added **in an adhoc manner**. »
> — `sdas/linksrc/aslink.h:321-329` (emphase ajoutée)

Extensions au-delà de 8 bits, bloc `/* sdld specific */`
(`sdas/linksrc/aslink.h:373-382`) :

```
#define R_BYT3  0x100           /* if R3_BYTE is set, this is a 3 byte address,
                                 * of which the linker must select one byte. */
#define R_HIB   0x200           /* If R3_BYTE & R_BYT3 are set, linker will
                                 * select byte 3 of the relocated 24 bit address. */
#define R_BIT   0x400           /* Linker will convert from byte-addressable
                                 * space to bit-addressable space. */
```

### 5.4 Portée 8 / 16 bits et PC-relative

- **16 bits (word)** : bit 0 à 0. Deux octets du T line patchés.
- **8 bits (byte)** : bit 0 à 1. Le bit 3 (`R3_BYTX`, 0x08) dit si la donnée
  occupe 1 ou 2 octets dans le T line ; le bit 7 (`R3_MSB`, 0x80) dit lequel des
  deux prendre. Documentation amont pour le cas multi-octets :
  « The 2nd byte format (also named MSB) always uses the second byte of the 2, 3,
  or 4-byte data. » (asls01.htm#Input3, « 24-Bit and 32-Bit Addressing »).
- **PC-relative** : bit 2 (`R3_PCR`, 0x04). Fonction du linker listée
  explicitement : « Perform byte and word program counter relative (pc or pcr)
  addressing calculations » (https://shop-pdp.net/ashtml/aslink.htm).
  Message d'erreur associé : `?ASlink-Warning-Byte PCR relocation error for
  symbol <nom>` (asls01.htm, « Linker V3 Error Messages »).
- **Signé / non signé** : bit 4 (`R3_USGN`). Erreur associée :
  `?ASlink-Warning-Unsigned Byte error for symbol <nom>`, « The Unsigned byte
  error indicates an indexing value was negative or larger than 255 » (idem).

### 5.5 L'amorce implicite de la ligne R

Le « `R 0 0 nn nn` » du début de ligne n'est pas un préambule spécial : c'est
une **première entrée de relocalisation ordinaire** de mode
`R_WORD|R_AREA` (= 0), d'index 0, dont la « cible » est l'area courante :

```c
/* sdas/asxxsrc/asout.c, outchk() */
	if (txtp == txt) {
		out_txb(a_bytes,dot.s_addr);
		if ((ap = dot.s_area) != NULL) {
                        write_rmode(R_WORD|R_AREA, 0);
			out_rw(ap->a_ref);
		}
	}
```

`out_rw` écrit **toujours 2 octets** (`sdas/asxxsrc/asout.c`, fonction `out_rw`),
indépendamment de `a_bytes`. Donc : **les index d'area et de symbole font
2 octets, y compris en mode 32 bits**, alors que l'offset du T line en fait 4.
Ce détail n'est pas énoncé dans `format.txt` ; il vient du code.

---

## 6. A5 — Encodage des symboles

`sdas/doc/format.txt:27-40` (texte identique amont, asls01.htm#Input3) :

> ```
>                 S string Defnnnn
>                         or
>                 S string Refnnnn
> ```
> « The symbol line defines (Def) or references (Ref) the symbol 'string' with the
> value nnnn. The defined value is relative to the current area base address.
> References to constants and external global symbols will always appear before
> the first area definition. References to external symbols will have a value of
> zero. »

Émission (`sdas/asxxsrc/asout.c:1402-1440`, `outsym()`) :

```c
	fprintf(ofp, "S ");
	fprintf(ofp, "%s", &sp->s_id[0]);
	fprintf(ofp, " %s", sp->s_type==S_NEW ? "Ref" : "Def");
	... case 4:	frmt = "%08lX\n"; break;   /* a_bytes == 4 */
```

Donc pour sdasz80 : `S _foo Def0000A3C4` — **8 chiffres hexadécimaux, collés au
mot `Def`/`Ref`** (pas d'espace).

**Ordre d'émission** (`sdas/asxxsrc/asout.c:1126-1157`) : d'abord les symboles
globaux sans area (constantes et références externes), puis, pour chaque area
dans l'ordre de son `a_ref`, la ligne `A` suivie des symboles globaux de cette
area. La numérotation `s_ref` est l'ordre d'émission ; c'est l'index utilisé par
`xx xx` dans les lignes R.

**Seuls les symboles globaux sont exportés.** `sdas/doc/README` (section NoICE) :
« Non-global symbols are not passed to the object file. » Dans la source
assembleur, `nom::` déclare un label global, `nom:` un label local
(`src/SDCCasm.c:404-405` : `{"labeldef", "%s::"}`, `{"slabeldef", "%s:"}`).

**Sensibilité à la casse.** Les symboles sont comparés avec le drapeau `zflag` :

```c
/* sdas/linksrc/lksym.c:241-244, lkpsym() */
        h = hash(id, zflag);
        ...
                if (symeq(id, sp->s_id, zflag))
```

`zflag` vaut 0 par défaut (`sdas/linksrc/lkdata.c:89`, « Disable symbol case
sensitivity ») et n'est mis à 1 que par l'option `-z`
(`sdas/linksrc/lkmain.c:1134`). L'aide du linker le confirme :

```
	"Case Sensitivity:",
	"  -z   Disable Case Sensitivity for Symbols",
```
— `sdas/linksrc/lkmain.c:1816-1854` (`usetxt_z80_gb[]`, texte affiché pour la
cible Z80).

**Donc : les symboles sont sensibles à la casse par défaut, les noms d'areas ne
le sont jamais.** L'asymétrie est réelle et vérifiée dans le code.

Le format **V6** amont ajoute des formes `S name =D nnnn` et
`S name =Rn^Z( expression )` pour passer des expressions complexes au linker
(asls01.htm#Input6, « Symbol Line »). **sdas ne les émet pas et sdld ne les lit
pas** (`outsym()` n'a que les deux formes `Def`/`Ref`).

---

## 7. A6 — Existe-t-il une notion de bank ?

C'est la question la plus importante pour le projet, et la réponse est nette.

### 7.1 Dans le format ASxxxx amont : oui, à partir de V4

Enregistrement `B` du format V6/V5/V4
(https://shop-pdp.net/ashtml/asls01.htm#Input6, « Bank Line ») :

> ```
>         B string base nn size nn map nn flags nn fsfx string
> ```
> « The B line defines a bank name as 'string'. A bank is a structure containing a
> collection of areas. The bank is treated as a unique linking structure seperate
> from other banks. Each bank can have a unique base address (starting address).
> The size specification may be used to signal the overflow of the banks'
> allocated space. The Linker combines all areas included within a bank as
> seperate from other areas. The code from a bank may be output to a unique file
> by specifying the File Suffix parameter (fsfx). This allows the seperation of
> multiple data and code segments into isolated output files. The map parameter is
> for NOICE processing. The flags indicate if the parameters have been set. »

Et l'appartenance d'une area à une bank passe par le champ optionnel `[bank bb]`
de la ligne A (même page, « Area Line »).

### 7.2 Dans sdas/sdld : non, pas pour le Z80

Trois constats, tous vérifiables :

1. **La directive `.bank` est commentée dans l'assembleur Z80 :**

   ```c
   /* sdas/asz80/z80pst.c:86-89 */
       {	NULL,	".area",	S_AREA,		0,	0	},
   //    {	NULL,	".psharea",	S_AREA,		0,	O_PSH	},
   //    {	NULL,	".poparea",	S_AREA,		0,	O_POP	},
   //    {	NULL,	".bank",	S_BANK,		0,	0	},
   ```

   Identique dans SDCC 4.5.0 (`sdas/asz80/z80pst.c:58`) et 4.6.0. Dans 4.6.0 les
   structures `bank[]` du fichier sont de plus enfermées dans
   `#ifdef ONLY_ASXXXX` (`sdas/asz80/z80pst.c:32`), c'est-à-dire compilées
   uniquement hors build SDCC.

2. **Le linker n'a aucun point d'entrée pour l'enregistrement `B` :** le
   `switch` de `link_main()` (`sdas/linksrc/lkmain.c:565-725`) ne comporte pas de
   `case 'B'`, et `newbank()` (`sdas/linksrc/lkbank.c:101`) n'est appelé nulle
   part dans `sdas/linksrc/*.c`. Le code de gestion des banks
   (`lkbank.c`, `struct bank` dans `aslink.h:618-637`) est compilé
   (`sdas/linksrc/Makefile.in:50`) mais inatteignable : seule la bank par défaut,
   anonyme, existe.

3. **Aucune option de ligne de commande de bank pour la cible Z80.** L'aide
   affichée par `sdldz80` (`sdas/linksrc/lkmain.c:1816-1854`, tableau
   `usetxt_z80_gb[]`) ne contient sous « Relocation: » que :

   ```
        "  -b   area base address = expression",
        "  -g   global symbol = expression",
   ```

   Il n'y a pas de `-b <bank>` : le `-b` de sdld place une **area**, pas une bank.

### 7.3 Le seul banking réellement implémenté dans sdld est celui de la Game Boy

```c
/* sdas/linksrc/lkarea.c:411-425, lnkarea() */
                if(TARGET_IS_GB && ap->a_addr == 0) {
                        if(!strncmp(ap->a_id, "_CODE_", 6) && atoi(ap->a_id+6)!=0) {
                                // set sane default values for rom banking
                                // 0x4000 is correct for MBC1,2,3,5,7
                                ap->a_addr = (atoi(ap->a_id+6) << 16) + 0x4000;
                        }
                        if(!strncmp(ap->a_id, "_DATA_", 6)) {
                                // set sane default values for ram banking
                                ap->a_addr = (atoi(ap->a_id+6) << 16) + 0xA000;
                        }
                }
```

Le numéro de bank est encodé dans les bits hauts de l'adresse 32 bits
(`bank << 16`), et la convention est purement **nominale** : c'est le *nom*
`_CODE_<n>` de l'area qui porte le numéro. Le garde est `TARGET_IS_GB` : ceci
**ne s'applique pas** à `sdldz80`.

### 7.4 Ce que SDCC, côté compilateur, fait du banking Z80

Le compilateur, lui, a un modèle de banking — qui vit **entièrement hors du
format objet**, dans des noms d'areas et des symboles absolus.

- `#pragma bank <n>` fabrique un nom d'area :
  ```c
  /* src/z80/main.c:507-511 */
              case ASM_TYPE_ASXXXX:
                dbuf_printf (&buffer, "CODE_%d", token.val.int_val);
  ```
  soit l'area `_CODE_<n>` après ajout du préfixe (§8.2). `#pragma bank BASE`
  donne `HOME` → area `_HOME` (`src/z80/main.c:533-536`).
- Les options `-bo <n>` / `-ba <n>` font de même pour code et données
  (`src/z80/main.c:732` : `"CODE_%u"` ; `:751` : `"DATA_%u"`). Le manuel les
  documente : « `-bo <Num>` Use code bank <Num>. » / « `-ba <Num>` Use data bank
  <Num>. » (`doc/sdccman.lyx:21178-21228`) — la note du manuel les rattache aux
  « GBZ80 Options ».
- Une fonction `__banked` fait émettre un **symbole absolu** portant le numéro de
  bank, de nom `b<rname>` :
  ```c
  /* src/z80/gen.c:8515-8527 */
    if (IFFUNC_BANKED (sym->type))
      {
        int bank_number = 0;
        for (int i = strlen (options.code_seg)-1; i >= 0; i--) { ... bank_number = atoi (...); }
        emit2("!bequ", sym->rname, bank_number);
      }
  ```
  avec `{"bequ", "b%s = %i"}` (`src/SDCCasm.c:452`). Donc pour `void f(void) __banked`
  compilée avec `--codeseg CODE_3`, l'objet contient un symbole `b_f = 3`.
- L'appel banké passe par un trampoline de bibliothèque, à fournir par
  l'utilisateur :
  ```c
  /* src/z80/gen.c:8222-8248 */
          /* there 3 types of banked call:
               legacy - only if --legacy-banking is specified
               a:bc - only for __z88dk_fastcall __banked functions
               e:hl - default (may have optimal bank switch routine) */
  ```
  ```asm
  ;; device/lib/z80/__sdcc_bcall.s, en-tête
  ; This file contains generic trampolines for banked function calls.
  ; They are not complete. Programmer must provide set_bank and get_bank
  ; routines. Or rewrite whole code completely.
  ```

**Réponse A6.** Le format `.rel` **tel que consommé par sdld** n'a **aucune**
notion de bank. La notion existe dans le format ASxxxx amont (enregistrement `B`,
champ `[bank bb]` de la ligne A, à partir de V4), et dans les structures de
données de `sdld`, mais elle est morte : ni émise par `sdasz80`, ni parsée par
`sdld`. Le banking SDCC/Z80 se réduit à (a) des conventions de **nommage
d'areas** (`_CODE_<n>`, `_DATA_<n>`), (b) un **symbole absolu** `b<nom>` par
fonction bankée, (c) un **trampoline** que l'utilisateur doit écrire, (d) pour la
seule cible Game Boy, un placement par défaut câblé dans `lkarea.c`.

C'est une bonne nouvelle pour le projet : la place est libre, et l'information
dont un linker a besoin (quelle area dans quelle bank) est **exprimable dans le
format existant** via le nom de l'area — sans extension.

---

## 8. B8 — Areas émises par SDCC pour le Z80

### 8.1 La table du port

Les noms sont dans la structure `PORT z80_port` (`src/z80/main.c:1297`), champ
`mem`, dont l'ordre des champs est défini par `struct { … } mem;` dans
`src/port.h:225-246` :

```c
/* src/z80/main.c:1344-1372 */
  {
    "XSEG",                     /* xstack_name  */
    "STACK",                    /* istack_name  */
    "CODE",                     /* code_name    */
    "DATA",                     /* data_name    */
    NULL,                       /* idata */
    NULL,                       /* pdata */
    NULL,                       /* xdata */
    NULL,                       // xconst
    NULL,                       /* bit */
    "RSEG (ABS)",               /* reg_name     */
    "GSINIT",                   /* static initialization */
    NULL,                       /* overlay */
    "GSFINAL",                  /* post_static_name */
    "HOME",                     /* home_name    */
    NULL,                       /* xidata */
    NULL,                       /* xinit */
    NULL,                       /* const_name */
    "CABS (ABS)",               // cabs_name
    NULL,                       // xabs_name
    "DABS (ABS)",               // iabs_name
    "INITIALIZED",              /* name of segment for initialized variables */
    "INITIALIZER",              /* name of segment for copies of initialized variables in code space */
```

### 8.2 Le préfixe `_` est ajouté par la table de mapping assembleur

```c
/* src/z80/mappings.i:33-38 */
static const ASM_MAPPING _asxxxx_z80_mapping[] = {
    /* We want to prepend the _ */
    { "area", ".area _%s" },
    { "areacode", ".area _%s" },
    { "areadata", ".area _%s" },
    { "areahome", ".area _%s" },
```

`_asxxxx_z80` combine cette table avec `asm_asxxxx_mapping`
(`src/z80/mappings.i:575-578` en 4.5.0 ; même construction en 4.6.0) et est
installée par `_z80_init()` (`src/z80/main.c`, `asm_addTree (&_asxxxx_z80)`).
Le même `.area _%s` vaut pour la SM83/Game Boy (`_asxxxx_gb_mapping`,
`src/z80/mappings.i:2-6`) et pour les Rabbit (`_asxxxx_r2k_mapping`, `:78-...`).

### 8.3 Liste effective et rôle de chacune

| Area émise | Origine | Rôle |
|---|---|---|
| `_CODE` | `code_name` | code et données constantes ; « CODE is read-only » (`src/z80/main.c:1369`) |
| `_HOME` | `home_name` | code non banké / « base » ; cible de `#pragma bank BASE` (`src/z80/main.c:533-536`) |
| `_GSINIT` | `static_name` | « static initialization » — code d'initialisation exécuté avant `main` |
| `_GSFINAL` | `post_static_name` | fin du bloc d'initialisation (contient le `ret`, voir `crt0.s`) |
| `_DATA` | `data_name` | données en RAM ; **mises à zéro** par `crt0.s` via `s__DATA` / `l__DATA` |
| `_INITIALIZED` | `initialized_name` | variables globales explicitement initialisées, en RAM |
| `_INITIALIZER` | `initializer_name` | copie en ROM des valeurs de `_INITIALIZED`, recopiée par `crt0.s` |
| `_CABS (ABS)` | `cabs_name` | données constantes à adresse absolue (`__at`) |
| `_DABS (ABS)` | `iabs_name` (4.6.0) / `xabs_name` (4.5.0) | données RAM à adresse absolue |
| `_RSEG (ABS)` | `reg_name` | pseudo-registres |
| `_XSEG`, `_STACK` | `xstack_name`, `istack_name` | présents dans la table ; non observés dans le flux Z80 courant — **non confirmé** |

Émission : `src/SDCCglue.c:152-169` (`!areacode` / `!areadata` / `!areahome` /
`!area` selon que `map->sname` vaut `CODE_NAME`, `DATA_NAME`, `HOME_NAME` ou
autre chose), `src/SDCCglue.c:2035-2074` pour `xinit` / `initializer` / `c_abs`.

Areas déclarées par la bibliothèque, pas par le compilateur
(`device/lib/z80/crt0.s`) :

```asm
	.area	_HEADER (ABS)     ; ligne 33 : vecteurs de reset, .org 0
	;; Ordering of segments for the linker.
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area   _GSINIT
	.area   _GSFINAL
	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area   _BSS
	.area   _HEAP
```

L'ordre des areas dans l'image liée est donc fixé par l'ordre de leur **première
apparition**, dans `crt0.rel`, qui doit être lié en premier — le manuel l'écrit :
« When using a custom crt0.rel it needs to be listed first when linking. »
(`doc/sdccman.lyx:49106`). `_HEAP` / `_HEAP_END` proviennent de
`device/lib/z80/heap.s:37,42`.

### 8.4 `_BSS` : le point à corriger dans la spec

**Le compilateur SDCC/Z80 n'émet jamais dans `_BSS`.** Une recherche de la chaîne
`BSS` sur `src/*.c`, `src/*.h` et `src/z80/*.c` (SDCC 4.6.0) ne donne **aucun
résultat**. Dans tout `device/lib/z80/`, `_BSS` n'apparaît qu'une fois, dans la
liste d'ordonnancement de `crt0.s:85` — une déclaration d'area vide, sans
contenu.

Le rôle qu'on attendrait de `_BSS` est tenu par `_DATA`, que `crt0.s` met à zéro
explicitement dans `_GSINIT` (bloc « Default-initialized global variables. »,
boucle `ldir` sur `l__DATA` / `s__DATA`). Pour comparaison, le port mos6502
utilise réellement `s_BSS` / `l_BSS` (`device/lib/mos6502-stack-auto/crt0.s:106-113`) :
la vestige `_BSS` du Z80 vient de là, pas d'un usage Z80.

### 8.5 Options de renommage

| Option | Définition | Effet |
|---|---|---|
| `--codeseg <name>` | `src/z80/main.c:36`, enregistrée ligne 94 | remplace `CODE` |
| `--constseg <name>` | `src/z80/main.c:37`, enregistrée ligne 95 | area des données constantes ; utilisée en `src/SDCCglue.c:1920-1921, 2035-2036, 2064-2070` |
| `--dataseg <name>` | `src/z80/main.c:38`, enregistrée ligne 96 | remplace `DATA` |
| `-bo <n>` / `-ba <n>` | `src/z80/main.c:34-35`, déclarées lignes 91-92, traitées lignes 726-753 | équivaut à `--codeseg CODE_<n>` / `--dataseg DATA_<n>` |
| `#pragma codeseg` / `#pragma constseg` | `src/z80/main.c`, `P_CODESEG`, `P_CONSTSEG` | idem, par unité de compilation |

Le manuel documente `--codeseg` et `--constseg` en des termes qui intéressent
directement le projet :

> « `<Name>` The name to be used for the code segment, default CSEG. This is
> useful if you need to tell the compiler to put the code in a special segment so
> you can later on tell the linker to put this segment in a special place in
> memory. **Can be used for instance when using bank switching to put the code in
> a bank.** »
> — `doc/sdccman.lyx:17831-17839` (et 17895-17903 pour `--constseg`)

Note : la valeur « default CSEG » du manuel est le défaut *générique*, pas celui
du Z80 ; pour le Z80 c'est `CODE`, donc l'area `_CODE`
(`src/z80/main.c:1347` + `src/SDCCmain.c:657`).

Autre mécanisme, à connaître : les **named address spaces non intrinsèques**
(`__addressmod`), destinés explicitement au bank switching :

> « SDCC supports user-defined non-intrinsic named address spaces. So far SDCC only
> supports them for bank-switching. […] Variables in non-intrinsic named address
> spaces will be placed in areas of the same name (this can be used for the
> placement of named address spaces in memory by the linker). SDCC will
> automatically insert calls to the corresponding function before accessing the
> variable. »
> — `doc/sdccman.lyx:25994-26160`

Là encore : le lien avec le linker se fait par le **nom de l'area**, rien d'autre.

---

## 9. B9 — Conventions d'appel Z80

Source primaire : `doc/sdccman.lyx`, section « Z80, Z180, Z80N and R800 calling
conventions », lignes 49854–49962 (SDCC 4.5.0 ; le texte est inchangé en 4.6.0).
Les figures citées par le manuel sont `doc/z80-arguments.svg` et
`doc/z80-stack-cleanup.svg`.

### 9.1 Défaut de la version courante

> « The current default is the SDCC calling convention, version 1. Using the
> command-line option `--sdcccall 0`, the default can be changed to version 0.
> There are three other calling conventions supported, which can be specified
> using the keywords `__smallc`, `__z88dk_fastcall` and `__z88dk_callee`. They are
> primarily intended for compatibility with libraries written for other
> compilers. »
> — `doc/sdccman.lyx:49860-49867`

Confirmé dans le code : le champ « ABI revision » de `z80_port` vaut `1`
(`src/z80/main.c:1374`), et `options.sdcccall = port->sdcccall`
(`src/SDCCmain.c:664`). L'option est déclarée en `src/z80/main.c:48` / :103.

L'ABI effective est **inscrite dans chaque objet** via la ligne `O` (§3.4) :
`.optsdcc -mz80 sdcccall(1)`. C'est le point de contrôle qu'un linker tiers doit
lire pour refuser un mélange incohérent.

### 9.2 `__sdcccall(1)` — la convention par défaut

**Valeur de retour** (`doc/sdccman.lyx:49883-49888`) :

> « 8-bit return values are passed in `a`, 16-bit values in `de`, 24-bit values in
> `lde`, 32-bit values in `hlde`. Larger return values (as well as struct and
> union independent of their size) are passed in memory in a location specified by
> the caller through a hidden pointer argument. »

**Passage des arguments, fonctions sans arguments variables**
(`doc/sdccman.lyx:49907-49921`) :

> « the first parameter is passed in `a` if it has 8 bits. If it has 16 bits it is
> passed in `hl`. If it has 32 bits, it is passed in `hlde`. If the first parameter
> is in `a`, and the second has 8 bits, it is passed in `l`; if the first is passed
> in `a` or `hl`, and the second has 16 bits, it is passed in `de`; all other
> parameters are passed on the stack, right-to-left. Independent of their size,
> struct / union parameters and all following parameters are always passed on the
> stack. »

Noter l'asymétrie, source d'erreurs : le **premier** argument 16 bits arrive en
`hl`, mais la **valeur de retour** 16 bits repart en `de`.

**Fonctions à arguments variables** (`doc/sdccman.lyx:49892-49894`) :

> « All parameters are passed on the stack. The stack is not adjusted for the
> parameters by the callee (thus the caller has to do this instead). »

**Qui dépile** (`doc/sdccman.lyx:49934-49939`) :

> « If `__z88dk_callee` is not used, after the call, the stack parameters are
> cleaned up by the caller, with the following exceptions: functions that do not
> have variable arguments and return void or a type of at most 16 bits, or have
> both a first parameter of type float and a return value of type float. »

Autrement dit : **le nettoyage est fait par l'appelé** dans le cas courant
(fonction non variadique retournant `void` ou ≤ 16 bits), et par l'appelant dans
les autres cas. C'est l'inverse de l'intuition « caller cleans up », et c'est
une source de bug classique quand on écrit de l'assembleur appelé depuis C.

### 9.3 `__sdcccall(0)` — l'ancienne convention

`doc/sdccman.lyx:49948-49961` :

> « All parameters are passed on the stack, right-to-left. 8-bit return values are
> passed in `l`, 16-bit values in `hl`, 24-bit values in `ehl`, 32-bit values in
> `dehl`. Except for the SM83, where 8-bit values are passed in `e`, 16-bit values
> in `de`, 32-bit values in `hlde`. Larger return values (as well as struct and
> union independent of their size) are passed in a memory in a location specified
> by the caller through a hidden pointer argument. Unless `__z88dk_callee` is used,
> all stack parameters are cleaned up by the caller. »

### 9.4 Registres sauvegardés

**Le manuel ne donne pas de liste de registres callee-saved pour le Z80.**
Recherche sur `preserve`, `clobber`, `callee-saved`, `must be saved` dans
`doc/sdccman.lyx` : les seules occurrences pertinentes sont la section
« Preserved register specification » et une remarque sur les fonctions `__naked`.

Ce que la documentation dit, et qui permet une inférence solide :

> « SDCC allows to specify preserved registers in function declarations, to enable
> further optimizations on calls to functions implemented in assembler. Example for
> the Z80 architecture specifying that a function will preserve register pairs `bc`
> and `iy`: `void f(void) __preserves_regs(b, c, iyl, iyh);` »
> — `doc/sdccman.lyx:26439-26450`

Et, du côté des options :

```c
/* src/z80/main.c:39 et :89 */
#define OPTION_CALLEE_SAVES_BC  "--callee-saves-bc"
  {0, OPTION_CALLEE_SAVES_BC, &z80_opts.calleeSavesBC, "Force a called function to always save BC"},
```

L'existence d'une annotation *opt-in* `__preserves_regs` et d'une option
*forçant* la sauvegarde de `BC` implique qu'**aucun** registre n'est préservé par
défaut. Je marque cette conclusion **inférée, non citée textuellement** : je n'ai
pas trouvé de phrase de la documentation officielle qui l'énonce.

Deux faits vérifiables complètent le tableau :

- `IX` est empilé/dépilé par le prologue/épilogue quand un frame pointer est
  utilisé : `{ "enter", "push\tix\nld\tix, #0\nadd\tix, sp" }`
  (`src/z80/mappings.i:64-67`). Le prologue peut être remplacé par un appel
  `___sdcc_enter_ix` (`{ "enters", "call\t___sdcc_enter_ix\n" }`, idem :68-69).
- `--reserve-regs-iy` existe pour interdire l'usage d'IY
  (`src/z80/main.c:43`, déclarée ligne 107 ; documenté `doc/sdccman.lyx:20273, 20295`), ce qui
  n'aurait pas de sens si IY était callee-saved.

### 9.5 Autres conventions, pour mémoire

- `__z88dk_fastcall` : « there may be only one parameter of at most 32 bits, which
  is passed the same way as the return value of `__sdcccall(0)` »
  (`doc/sdccman.lyx:49868-49870`).
- `__z88dk_callee` : « the stack is not adjusted for stack parameters […] after the
  call (thus the callee has to do this instead) » (`:49871-49872`) ; combinable
  avec `__smallc`, `__sdcccall(0)` ou `__sdcccall(1)` (`:49873-49874`).
- `__smallc` : « passing arguments on-stack left-to-right, 1 byte arguments are
  passed as 2 bytes, with the value in the lower byte. 8-bit return values are
  passed in `a`, 16-bit values in `de`, 32-bit values in `hlde` »
  (`doc/sdccman.lyx:50148-50154`).

Les mots-clés reconnus par le port z80 sont listés en `src/z80/main.c:183-198`
(SDCC 4.6.0) : `sfr`, `nonbanked`, `banked`, `at`, `_naked`, `critical`,
`interrupt`, `z88dk_fastcall`, `z88dk_callee`, `smallc`, `dynamicc`,
`z88dk_shortcall`, `z88dk_params_offset`. (`dynamicc` est apparu entre 4.5.0 et
4.6.0 ; il n'est pas documenté dans la section « calling conventions » du manuel
consultée — **non trouvé**.)

---

## 10. B10 — Mangling des symboles

### 10.1 Préfixe underscore

Le port Z80 déclare `fun_prefix = "_"` :

```c
/* src/z80/main.c:1390 (bloc de PORT z80_port) */
  "_",
```

Le champ est documenté dans `src/port.h:326-327` :

```c
  /** Prefix to add to a C function (eg "_") */
  const char *fun_prefix;
```

Application : `src/SDCCast.c:7607`
(`SNPRINTF (name->rname, sizeof (name->rname), "%s%s", port->fun_prefix, name->name)`)
et `src/SDCCmem.c:526` pour les variables. Un identifiant C `main` devient donc
le symbole assembleur `_main` — ce que confirme `device/lib/z80/crt0.s:31`
(`.globl	_main`).

Cas particuliers observés :
- Les paramètres remontés en mémoire prennent la forme
  `<fun_prefix><fonction>_PARM_<n>` (`src/SDCCmem.c:714`).
- Les champs de structure reçoivent un `_` **câblé** et non `fun_prefix`
  (`src/SDCCsymt.c:1731`) : sans effet ici puisque `fun_prefix == "_"` pour le
  Z80, mais à noter.
- Une fonction `__banked` reçoit en plus un symbole `b<rname>`, soit `b_f` pour
  `f` (§7.4).
- Le port ISAS (Game Boy) met `fun_prefix = ""` (`src/z80/main.c:696` en 4.5.0) :
  le préfixe est donc bien un choix de port/dialecte, pas une propriété du
  format.

### 10.2 Casse

Les identifiants C sont recopiés **tels quels**, sans changement de casse : aucune
transformation de casse n'apparaît dans les chemins ci-dessus. Le linker les
compare en respectant la casse par défaut (§6, `zflag`).

### 10.3 Longueur

`sdas/doc/README` (section « MISCELLANEOUS CHANGES ») :

> « I have modified the assembler and linker to allow names up to 80 characters,
> moving the name strings out of the sym struct. »

Cohérent avec `NCPS` dans les sources sdas. Je n'ai pas vérifié la valeur exacte
de `NCPS` en 4.6.0 — **non confirmé au chiffre près**.

### 10.4 Portée locale

`nom::` = symbole global exporté, `nom:` = symbole local non exporté
(`src/SDCCasm.c:404-405`, `{"labeldef", "%s::"}` / `{"slabeldef", "%s:"}` ; et
pour les fonctions, `{"functionlabeldef", "%s:"}` vs
`{"globalfunctionlabeldef", "%s::"}`, `src/SDCCasm.c:435-436`, choisis selon
`IS_STATIC` en `src/z80/gen.c:8529-8532`).
Les labels temporaires sont numériques : `{"tlabeldef", "%05d$:"}`.

---

## 11. A7 — Le format est-il une interface stable et documentée ?

**Documentée : oui. Stable et garantie pour des tiers : rien ne le dit, et les
faits suggèrent le contraire.**

Éléments en faveur d'une interface publique :

- Le format a un numéro de version explicite (V3/V4/V5/V6), un chapitre de
  documentation dédié, et le linker amont promet l'interopérabilité entre
  versions : « Object files from version 3, 4, 5, and 6 may be freely mixed while
  linking. » (https://shop-pdp.net/ashtml/aslink.htm).
- SDCC redistribue une description du format dans son propre arbre
  (`sdas/doc/format.txt`).

Éléments contre :

1. **Le format a changé de largeur d'adresse dans une version mineure de SDCC.**
   « In 4.4.1, the address width in .rel files was increased from 24 bits to
   32 bits for the z80 (and related) ports. » (`doc/sdccman.lyx:3114-3115`).
   Un lecteur écrit pour SDCC 4.3 lit faux les objets de SDCC 4.4.1.
2. **sdas ajoute des champs et des enregistrements non documentés dans la spec
   amont** : le champ `addr` de la ligne A (`asout.c:1349-1375`) et
   l'enregistrement `O` (`asout.c:1180`), tous deux gardés par `is_sdas()`.
3. **sdas ajoute un mécanisme d'échappement au champ de mode** de relocalisation
   (`R_ESCAPE_MASK`, `asout.c:325-352`), documenté seulement dans
   `sdas/doc/format.txt` — pas dans `asls01.htm`.
4. **Les extensions de mode sont explicitement décrites comme ad hoc** par le
   code lui-même : « the extended modes […] are being added **in an adhoc
   manner** » (`sdas/linksrc/aslink.h:326-329`).
5. **Une même chaîne se comporte différemment selon la cible.** Le champ `addr`
   est lu pour 8051/hc08 et ignoré pour Z80/Z180/GB (`lkarea.c:156-165`) ; le
   banking `_CODE_<n>` n'existe que pour GB (`lkarea.c:411-425`). Le format seul
   ne suffit pas : il faut savoir quelle cible l'a produit.
6. **Le renumérotage de version interne entre SDCC 4.5.0 et 4.6.0**
   (`"V03.00/V05.40 + sdld"` → `"V05.50.4-SDLD"`) montre que la base amont est
   resynchronisée par vagues, sans engagement de compatibilité annoncé.

**Aucune source primaire n'a été trouvée qui présente le `.rel` comme une
interface stable destinée à des outils tiers**, ni qui promette une
compatibilité arrière. Aucune ne le présente non plus comme un détail interne
volontairement instable. Le statut réel est intermédiaire : *format documenté,
sans contrat*.

Conséquence de conception : un lecteur de `.rel` doit **valider la ligne de
format `[XDQ][HL][234]`** au lieu de supposer `XL4`, et **tolérer les champs
inconnus** en fin de ligne A et les lettres d'enregistrement inconnues — ce que
fait déjà `sdld` avec son `default: break;` (`lkmain.c`).

---

## 12. Ce qui reste inconnu ou non vérifié

Ces points ne sont **pas** comblés par une supposition :

1. **Registres callee-saved du Z80** : aucune source primaire n'énonce
   explicitement que rien n'est préservé. La conclusion du §9.4 est une
   inférence à partir de `__preserves_regs` et `--callee-saves-bc`.
2. **`_XSEG` et `_STACK`** : présents dans la table du port
   (`src/z80/main.c:1345-1346`) mais je n'ai pas établi dans quelles conditions
   ils apparaissent dans un `.asm` Z80. Rôle non confirmé.
3. **Longueur maximale exacte d'un symbole** (`NCPS`) en SDCC 4.6.0 : non
   vérifiée ; `sdas/doc/README` parle de 80 caractères.
4. **Comportement de `sdld` sur des adresses > 0xFFFF pour la cible Z80** : le
   format est en 32 bits (`XL4`) et le banking GB encode la bank dans les bits
   hauts ; je n'ai pas vérifié ce que fait `sdldz80` d'une area placée
   au-delà de 0xFFFF. Non testé (aucun binaire SDCC installé sur cette machine ;
   toute l'analyse est faite sur le source).
5. **Aucune exécution réelle** : je n'ai pas pu produire un `.rel` de référence
   avec `sdcc`/`sdasz80` (absents de la machine — `which sdcc sdasz80 sdldz80` :
   introuvables). Les formes de lignes citées sont dérivées des `fprintf` du
   source, pas d'un fichier observé. **Il est fortement recommandé de produire
   un `.rel` témoin et de le comparer à ce document avant d'écrire un lecteur.**
6. **Contradiction résiduelle, signalée telle quelle** : `sdas/doc/asmlnk.txt`
   (section 1.4.22) attribue la génération de `s_<area>` / `l_<area>` aux
   *assembleurs*, alors que dans SDCC c'est le *linker* qui les crée
   (`lkarea.c:467-491`) et qu'aucune trace n'en existe côté sdas. La
   documentation amont est ici inexacte pour le fork SDCC.
7. **La documentation ASxxxx amont consultée (6.10, juillet 2026) est en avance
   de plusieurs versions majeures sur celle livrée par SDCC (5.05, août 2012).**
   Les sections V3 citées sont identiques mot pour mot entre les deux, ce que
   j'ai vérifié pour la ligne A, la ligne S, la ligne R et la ligne P. Je n'ai
   pas comparé le reste.

---

## 13. Verdict sur la spec

Reprise une à une des affirmations de la sous-section « Interopérabilité avec
SDCC » de `docs/spec-chaine-outils.md`.

### 13.1 « c'est **le linker de fantams qui doit lire les objets de SDCC**, non l'inverse »

**Confirmé** (comme conclusion technique ; la décision reste un choix de projet).
Les faits qui l'appuient : sdld ne lit que le format V3
(`lkmain.c:596`, `lkrloc.c:81-90`), n'a aucune notion de bank utilisable (§7),
et n'offre aucune option de banking pour la cible Z80
(`lkmain.c:1816-1854`). Il n'y a donc rien à gagner à passer par lui.

### 13.2 « SDCC/Z80 produit des `.rel` au format ASxxxx »

**À nuancer.** SDCC/Z80 produit des `.rel` au **format ASxxxx V3 augmenté par
sdas**, en variante `XL4` (hexadécimal, little-endian, adresses 32 bits) depuis
SDCC 4.4.1.

Formulation corrigée proposée :

> SDCC/Z80 produit des `.rel` au format ASxxxx **version 3**, dans le dialecte
> propre à `sdas` : ligne de format `XL4` (hexadécimal, octet de poids faible en
> tête, adresses 32 bits depuis SDCC 4.4.1), champ `addr` supplémentaire sur la
> ligne A, enregistrement `O` portant `.optsdcc -mz80 sdcccall(N)`, et
> échappement `0xfX` permettant des modes de relocalisation sur 12 bits. Les
> enregistrements `B` (bank) et `G` (merge mode) des versions 4 à 6 du format ne
> sont ni émis ni lus.

### 13.3 « des *areas* avec attributs, une table de symboles, des enregistrements de relocalisation »

**Confirmé.** Ligne `A label size ss flags ff [addr aa]`, lignes
`S <nom> Def|Ref<valeur>`, lignes `T`/`R` appariées. Voir §4, §5, §6.

Précision utile : les attributs d'area disponibles pour le Z80 sont **uniquement**
`ABS`/`REL`, `OVR`/`CON`, `PAG`/`NOPAG` (`sdas/asz80/z80pst.c:72-77`) — trois
bits. Pas de type mémoire, pas d'alignement, pas de `NOLOAD`. Le modèle est plus
pauvre que ce que « areas avec attributs » laisse imaginer.

### 13.4 « et les lie avec `sdld` »

**Confirmé.** `src/z80/main.c` déclare `sdldz80` comme linker du port
(`_z80_linkCmd[]` = `{ "sdldz80", "-nf", "$1", "$L", NULL }`,
`src/z80/main.c:1249`), et lui passe `-b_CODE=0x%04X -b_DATA=0x%04X`
(`src/z80/main.c:923`).

### 13.5 « dont la gestion du banking CPC est le point faible »

**Réfuté dans la formulation, confirmé en substance — mais bien plus fortement
que la spec ne le dit.** Ce n'est pas un « point faible » : il n'y a **rien du
tout**.

Formulation corrigée proposée :

> `sdld` n'a **aucune** gestion du banking pour la cible Z80. La directive
> `.bank` est commentée dans `sdasz80` (`sdas/asz80/z80pst.c:89`) ;
> l'enregistrement `B` du format ASxxxx V4+ n'est pas dispatché par le linker
> (`sdas/linksrc/lkmain.c`, `link_main()`) et `newbank()` n'est appelé de nulle
> part ; l'aide de `sdldz80` n'offre aucune option de bank. Le seul banking
> implémenté dans `sdld` est câblé pour la Game Boy, sous garde `TARGET_IS_GB`
> (`sdas/linksrc/lkarea.c:411-425`), et encode le numéro de bank dans les bits
> hauts de l'adresse d'après le *nom* de l'area (`_CODE_<n>`, `_DATA_<n>`).
> Côté compilateur, le banking Z80 de SDCC se réduit à un nom d'area
> (`#pragma bank <n>` → `_CODE_<n>`), à un symbole absolu `b<nom>` par fonction
> `__banked`, et à un trampoline `___sdcc_bcall*` que « the programmer must
> provide » (`device/lib/z80/__sdcc_bcall.s`).

### 13.6 « Un linker qui connaît `RMR`, `&DF00` et `11pppccc` est exactement ce qui manque à cette chaîne »

**Confirmé** au sens où le manque est réel et total (§13.5). Le contenu
Amstrad-spécifique de l'affirmation n'est pas du ressort de cette recherche.

Point positif non anticipé par la spec : **l'information « quelle area dans
quelle bank » est déjà exprimable dans le format existant**, par le nom de
l'area (`_CODE_3`) et par le symbole `b_<fonction>`. Un linker fantams peut donc
consommer les objets SDCC bankés **sans extension du format**.

### 13.7 « Émettre du `.rel` pour se faire lier par `sdld` nous soumettrait au contraire à ses limites. »

**Confirmé.** Les limites sont nommables : format V3 seul, aucune bank, trois
bits d'attribut d'area, aucun mécanisme d'alignement, champ `addr` ignoré pour
le Z80, et l'ordre des areas dans l'image déterminé par l'ordre de première
apparition dans les objets (d'où l'obligation de lier `crt0.rel` en premier,
`doc/sdccman.lyx:49106`).

### 13.8 « le modèle d'objet doit rester *alignable* sur le modèle ASxxxx — une section ≈ une *area* »

**Confirmé**, avec deux réserves à ajouter à la spec :

1. **Les noms d'areas ne sont jamais sensibles à la casse** (documentation :
   `sdas/doc/asmlnk.txt` §1.4.22, « The .area names and options are never case
   sensitive » ; code : `lkarea.c:223` avec `symeq(..., 1)`), alors que les
   **symboles le sont** par défaut (`lksym.c:244` avec `zflag`, mis à 1 seulement
   par `-z`). Un modèle de sections fantams sensible à la casse ne se projette
   pas fidèlement.
2. **Le contrat inclut les symboles `s_<area>` / `l_<area>`.** `sdld` les définit
   (`lkarea.c:467-491`) et `crt0.s` en dépend (`s__DATA`, `l__DATA`,
   `s__INITIALIZED`, `s__INITIALIZER`, `l__INITIALIZER`). Un linker qui lit du
   `.rel` SDCC **doit** les fabriquer. La spec ne le mentionne pas : **manque à
   ajouter.**

### 13.9 « les types `"ro"` / `"rw"` / `"uninit"` se projettent sur les conventions `_CODE` / `_DATA` / `_BSS` de SDCC »

**Réfuté sur `_BSS`.** Le compilateur SDCC/Z80 **n'émet jamais** dans `_BSS` : la
chaîne `BSS` est absente de tout `src/` (SDCC 4.6.0), et `_BSS` n'apparaît dans
`device/lib/z80/` que comme déclaration d'area vide dans la liste
d'ordonnancement de `crt0.s:85`.

Formulation corrigée proposée :

> Les types `"ro"` / `"rw"` / `"uninit"` se projettent sur les areas SDCC/Z80
> comme suit :
> - `ro` → `_CODE` (code et constantes ; `_HOME` pour le code non banké,
>   `_CABS (ABS)` pour les constantes à adresse fixe) ;
> - `rw` initialisé → **`_INITIALIZED`** en RAM, avec sa copie ROM dans
>   **`_INITIALIZER`**, recopiée par `crt0.s` ;
> - `uninit` → **`_DATA`**, que `crt0.s` met à zéro via `s__DATA` / `l__DATA`.
>
> `_BSS` est une area vestigiale : déclarée dans `crt0.s` pour fixer l'ordre des
> sections, jamais alimentée par le compilateur Z80. Ne pas construire de
> correspondance dessus.

À noter aussi : **le modèle SDCC a besoin de trois types là où la spec en pose
deux**, à cause du couple `_INITIALIZED` / `_INITIALIZER` — une paire
« destination RAM » + « image ROM » que ni `ro` ni `rw` ne décrit seul. Si le
modèle d'objet de fantams ne sait pas exprimer « cette section RAM est initialisée
depuis cette section ROM », la traduction depuis SDCC perd de l'information.
C'est le point le plus concret à trancher avant de s'engager.

### 13.10 « Ne pas inventer un modèle plus riche que ce que `.rel` sait exprimer, sous peine de ne pouvoir traduire que dans un sens. »

**Confirmé**, et le plafond est plus bas que la spec ne le suppose : trois bits
d'attribut d'area, pas de bank, pas d'alignement, pas d'expression complexe (les
formes `S name =R…` du V6 ne sont ni émises ni lues par sdas/sdld), pas de type
mémoire pour le Z80.

Nuance à ajouter cependant : la contrainte porte sur ce qu'on veut **réémettre**
en `.rel`. Puisque la conception retenue est de **lire seulement** (§13.1), un
modèle interne plus riche ne coûte rien, tant qu'il sait recevoir un `.rel`
appauvri. La phrase gagnerait à distinguer les deux : *le modèle d'objet peut
être plus riche que `.rel` ; c'est l'émission de `.rel` qui serait à proscrire,
et elle n'est pas au programme.*

---

## 14. Récapitulatif du verdict

| # | Affirmation | Verdict |
|---|---|---|
| 13.1 | fantams lit SDCC, pas l'inverse | confirmé |
| 13.2 | `.rel` « au format ASxxxx » | à nuancer (V3 + dialecte sdas, `XL4`) |
| 13.3 | areas à attributs, symboles, relocalisations | confirmé (attributs Z80 : 3 bits) |
| 13.4 | liés par `sdld` | confirmé |
| 13.5 | banking CPC = « point faible » de sdld | réfuté dans la forme : banking Z80 **absent**, pas faible |
| 13.6 | un linker qui connaît le banking CPC est ce qui manque | confirmé |
| 13.7 | émettre du `.rel` nous soumettrait aux limites de sdld | confirmé |
| 13.8 | section ≈ area | confirmé, + casse des noms d'areas, + `s_`/`l_` obligatoires |
| 13.9 | `ro`/`rw`/`uninit` → `_CODE`/`_DATA`/`_BSS` | **réfuté sur `_BSS`** ; il faut `_INITIALIZED` + `_INITIALIZER` |
| 13.10 | ne pas inventer plus riche que `.rel` | confirmé, à nuancer (ne vaut que pour l'émission) |
