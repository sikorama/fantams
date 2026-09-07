# Pagination ZX Spectrum 128 / +2A / +3 et MSX — vérification documentaire

Ce document vérifie, sur sources primaires quand elles existent, les valeurs
matérielles citées de mémoire dans `docs/spec-chaine-outils.md` § 13.2 (encadré
« À vérifier avant d'écrire ces profils »). Il ne propose pas de profil : il
établit les faits, et se termine par un verdict ligne à ligne sur les deux blocs
`TARGET zx128` et `TARGET msx-konami`.

Convention de lecture :

- **primaire** — manuel constructeur, service manual, documentation technique
  officielle (Sinclair, Amstrad, ASCII/Sony/Microsoft).
- **primaire-pour-le-modèle** — code source d'émulateur de référence (Fuse,
  openMSX). Ce n'est pas la parole du constructeur, mais c'est un modèle du
  matériel réel, souvent mesuré sur machine, et il documente ce que les manuels
  taisent. Signalé comme tel partout.
- **secondaire** — wiki, FAQ, forum, billet, page de reverse engineering. Toute
  affirmation qui ne repose que là-dessus est marquée **non confirmée**.

Ce qui n'a pas été trouvé est écrit comme non trouvé. Aucune valeur de bit n'est
déduite, extrapolée ou complétée.

---

## Table des sources

### ZX Spectrum

| Réf | Source | Niveau |
|-----|--------|--------|
| S1 | **ZX Spectrum 128 Service Manual**, Sinclair Research, réf. SR1AAA — <https://worldofspectrum.net/pub/sinclair/technical-docs/ZXSpectrum128K_TechnicalManual.pdf> | primaire |
| S2 | Même manuel, édition recomposée par Brendan Alford — <https://spectrumforeveryone.com/wp-content/uploads/2017/11/ZX-Spectrum-128-Service-Manual.pdf> | primaire pour le corps de texte ; **secondaire** pour les notes de l'éditeur, signalées comme telles |
| S3 | **ZX Spectrum +3 Manual** (Amstrad), ch. 8 partie 24 « The memory », section *Memory management* — <https://worldofspectrum.org/ZXSpectrum128+3Manual/chapter8pt24.html> | primaire |
| S4 | **ZX Spectrum +3 Manual**, ch. 8 partie 30 « Reference section — Hardware » — <https://worldofspectrum.org/ZXSpectrum128+3Manual/chapter8pt30.html> | primaire |
| S5 | **ZX Spectrum +3 Manual**, ch. 8 partie 27 « Guide to +3DOS » — <https://worldofspectrum.org/ZXSpectrum128+3Manual/chapter8pt27.html> | primaire |
| S6 | **ZX Spectrum +3 Manual**, ch. 8 partie 25 « The system variables » — <https://worldofspectrum.org/ZXSpectrum128+3Manual/chapter8pt25.html> | primaire |
| S7 | comp.sys.sinclair FAQ, *128K ZX Spectrum Reference* — <https://worldofspectrum.org/faq/reference/128kreference.htm> | secondaire |
| S8 | comp.sys.sinclair FAQ, *48K ZX Spectrum Reference* — <https://worldofspectrum.org/faq/reference/48kreference.htm> | secondaire |
| S9 | Fuse, `machines/spec128.c` — <https://sourceforge.net/p/fuse-emulator/fuse/ci/master/tree/machines/spec128.c> | primaire-pour-le-modèle |
| S10 | Fuse, `machines/specplus3.c` — <https://sourceforge.net/p/fuse-emulator/fuse/ci/master/tree/machines/specplus3.c> | primaire-pour-le-modèle |
| S11 | Fuse, `machines/machines_periph.c` (tables de masques de port) — <https://sourceforge.net/p/fuse-emulator/fuse/ci/master/tree/machines/machines_periph.c> | primaire-pour-le-modèle |
| S12 | Sinclair Wiki, *Memory paging* — <https://sinclair.wiki.zxnet.co.uk/wiki/Memory_paging> | secondaire |
| S13 | Sinclair Wiki, *ZX Spectrum 128* — <https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_128> | secondaire |
| S14 | Sinclair Wiki, *ZX Spectrum +3/2A/2B* — <https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_+3/2A/2B> | secondaire |
| S15 | Sinclair Wiki, *Contended memory* — <https://sinclair.wiki.zxnet.co.uk/wiki/Contended_memory> | secondaire |
| S16 | Spectrum For Everyone, *Spectrum compatibility issues* — <https://spectrumforeveryone.com/technical/spectrum-compatibility-issues/> | secondaire |
| S17 | **Amstrad +2A/+3 Service Manual** (scan) — <https://zxnet.co.uk/spectrum/schematics/plus3%20service%20manual.pdf> | primaire, mais **inexploitable** : scan sans couche texte |
| S18 | **Spectrum +3 Service Manual**, texte HTML — <https://worldofspectrum.org/ZXSpectrum128+3ServiceManual/> | primaire, mais **ne traite pas la pagination** (manuel de réparation) |

Limite du corpus primaire côté +2A/+3 : le seul document constructeur exploitable
en texte sur la pagination des +2A/+3 est le **manuel utilisateur +3, chapitre 8**
(S3, S4). Le service manual Amstrad (S17) est un scan image ; le service manual
+3 disponible en HTML (S18) est un document de diagnostic qui ne mentionne ni
`&7FFD` ni `&1FFD`. Toute question de décodage fin sur +2A/+3 tombe donc en
secondaire ou en primaire-pour-le-modèle.

### MSX

| Réf | Source | Niveau |
|-----|--------|--------|
| M1 | **MSX Technical Data Book**, ASCII / Sony / Microsoft, © 1984 — <https://map.grauw.nl/resources/system/msxtech.pdf> | primaire (scan sans OCR ; pages citées = pagination imprimée, page PDF = imprimée + 3) |
| M2 | **MSX2 Technical Handbook**, ASCII 1987, transcription Konamiman — <https://konamiman.github.io/MSX2-Technical-Handbook/> | primaire (transcription du livre ASCII) |
| M3 | **MSX-DOS 2 Program Interface Specification**, ASCII Corp., 26/11/1986, v2.00 — <https://archive.org/download/msx-dos-2-command-specification-1986-11-26-version-2.00/MSX-DOS_2_Program_Interface_Specification_1986-11-26_version_2.00.pdf> | primaire |
| M4 | openMSX, branche `master` — <https://github.com/openMSX/openMSX> | primaire-pour-le-modèle |
| M5 | MSX Assembly Page, *MSX I/O ports* — <https://map.grauw.nl/resources/msx_io_ports.php> | secondaire (de haute qualité) |
| M6 | « Megabit ROM Cartridges », BiFi / Sean Young, msxnet — <http://web.archive.org/web/20250718120922/http://bifi.msxnet.org/msxnet/tech/megaroms> | secondaire, mais reverse engineering explicitement vérifié sur cartouche physique |

Deux sources n'ont **pas** pu être exploitées, et rien n'en est tiré : le
**MSX Datapack vol. 3** (chapitre matériel), dont seuls des scans japonais sans
couche texte existent
(<https://archive.org/download/MSXDatapackVolume3_turboR/MSX-Datapack_Volume3_1991.pdf>) ;
et le wiki msx.org, qui a renvoyé HTTP 403.

---

## A. ZX Spectrum 128 / +2 / +2A / +3

### A.1 Port `&7FFD` — champ de bits (128 et +2)

**Source primaire, citation littérale.** S1/S2, § « MEMORY ORGANISATION » ¶ 4.3,
paragraphe qui commence par « The Z80 address space is allocated according to the
two most significant bits of the address bus (ZA14,15) and the contents of the
bank register IC31 which is at address 7FFDH in the Z80's I/O space » :

```
Bits      Function
B2-B0     Selects the page occupying the top 16K of the Z80 address space.
          Any RAM page can occupy the space.
B3        Instructs the ULA to access the display mapped in page 5 or 7.
              Bit set   : screen in page 7
              Bit clear : screen in page 5
B4        Determines whether instruction fetches are from ROM 0 or ROM 1
              Bit set   : fetches from the 48K Spectrum ROM (ROM 1)
              Bit clear : fetches from the 128K Spectrum ROM (ROM 0)
B5        Set to prevent further accesses to the bank register
          (protection against SPECTRUM programs crashing if the
          bank register is written to in error)
```

Donc, pour les modèles **128 et +2** :

| Bit | Fonction | Source |
|-----|----------|--------|
| 0–2 | numéro de banque RAM (0–7) en `&C000`–`&FFFF` | S1/S2 ¶ 4.3 (primaire) ; S7 § Memory (secondaire, concordant) |
| 3 | écran affiché par l'ULA : **0 → page 5** (écran normal), **1 → page 7** (shadow) | S1/S2 ¶ 4.3 et § 5.3.3 (primaire) ; S7 (secondaire, concordant) |
| 4 | ROM en `&0000` : **0 → ROM 0 = ROM 128K**, **1 → ROM 1 = ROM 48K** | S1/S2 ¶ 4.12.2 (primaire) |
| 5 | verrou de pagination | S1/S2 ¶ 4.3 et ¶ 4.12.12 (primaire) |
| 6–7 | **aucune fonction : non mémorisés** | S1/S2 ¶ 4.12.11 (primaire) |

Le sens du bit 4 est explicite dans S1/S2 ¶ 4.12.2 : « ROM 1 is the old 48K
Spectrum ROM (slightly modified) and is selected when bank register bit 4 sets
address A14. ROM 0 is the new Spectrum 128 ROM and is selected when bit 4 is
clear. » Fuse code la même chose (S9, `spec128_memory_map` :
`rom = ( ram.last_byte & 0x10 ) >> 4;`).

Le fait que seuls six bits existent est **matériel** : S1/S2 ¶ 4.12.11 — « The
register is positive edge triggered and latches D5-D0 off the data bus on the
negative (trailing) edge of the BANK output from the PAL IC29 » — et la
nomenclature du même manuel donne **IC31 = 74LS174**, un hex D flip-flop, soit
six bascules. Le détail du câblage broche par broche est décrit par S12
(secondaire, **non confirmé sur primaire** dans ce détail) : D0–D5 vers les
entrées D1–D6 du 74LS174, Q1–Q3 vers B0–B2 du HAL10H8, Q4 vers l'entrée VB de
l'ULA, Q5 vers l'entrée A14 de la ROM.

**Le verrou est-il définitif jusqu'au reset ? Oui — source primaire, citation
littérale.** S1/S2 ¶ 4.12.12 :

> « On selecting the 48K Spectrum mode, the Z80 writes a '1' into bit 5 of the
> register, thus preventing any further access. This action preserves the Z80
> address space, preventing erroneous calls to address 7FFDH crashing the
> SPECTRUM program. **The bit can only be cleared by using the RESET pushbutton
> or by interrupting the power supply input.** »

Le manuel +3 (S3, primaire) le confirme pour les +2A/+3 : « D5 is a safety
feature — once this bit has been set, no further paging operations will work…
It cannot be turned back into a 128K machine other [than] by switching off or
pressing RESET button ; however, the sound chip can still be driven by OUT. »

Le mécanisme physique est un **verrouillage de l'horloge du registre**, pas un
drapeau logiciel : S12 (secondaire, non confirmé sur primaire) décrit la sortie
Q6 ramenée sur l'entrée CLK du 74LS174 à travers une diode, l'entrée CLEAR étant
reliée à la ligne RESET. Conséquence pratique, si l'on suit cette description :
aucune séquence logicielle ne peut déverrouiller, et une NMI ne déverrouille pas
non plus (aucune source ne dit le contraire ; l'entrée CLEAR n'est reliée qu'à
/RESET).

**État après reset — primaire.** S1/S2 ¶ 4.4 : « On power up, or after reset the
bank register is cleared and loads page 0 at address C000H, selects the 128K
Spectrum ROM at address 0000H and informs the ULA that screen accesses are from
page 5. » Fuse fait de même (S9, `spec128_common_reset` : `last_byte = 0`,
`current_page = 0`, `current_rom = 0`, `memory_current_screen = 5`, `locked = 0`).

### A.2 Décodage du port, alias, écriture seule

#### 128 et +2 : seuls A15 et A1 sont décodés

**Primaire**, S1/S2 ¶ 4.12.11, citation littérale :

> « Bank Register (IC31) : The bank register is at address 7FFDH in the Z80
> address space… BANK is decoded (set high) from /IORQ and RD/WR active low
> (**I/O read or write cycle**) and ZA1 and ZA15 low (address 7FFDH). »

Le registre répond donc à **toute** adresse d'entrée-sortie telle que
`A15 = 0` et `A1 = 0`, soit le motif `0xxx xxxx xxxx xx0x` — 16 384 adresses
distinctes. Concordance : S7 (secondaire) « the port address is in fact only
partially decoded and the hardware will respond to any port address with bits 1
and 15 reset » ; S12 et S13 (secondaires) donnent le même motif ; et Fuse (S11,
primaire-pour-le-modèle) code exactement cela dans `spec128_memory_ports` :

```c
{ 0x8002, 0x0000, NULL, spec128_memoryport_write },   /* (port & 0x8002) == 0 */
```

Le gestionnaire de lecture est `NULL` — voir plus bas.

**Conséquence importante pour un profil.** Sur un 128 / +2, les quatre adresses
matérielles des +2A/+3 — `&0FFD` (imprimante), `&1FFD` (pagination étendue),
`&2FFD` (statut FDC), `&3FFD` (données FDC) — **alias toutes `&7FFD`**. Les ports
AY `&BFFD` et `&FFFD` sont hors d'atteinte (A15 = 1). S7 (secondaire) en tire la
recommandation d'usage : « 0x7ffd should be used if at all possible to avoid
conflicts with other hardware. »

#### +2A / +3 : décodage différent — pas de source primaire

**Non trouvé en primaire.** Le manuel +3 (S3, S4) donne les adresses nominales
`7FFDh` et `1FFDh` et ne dit jamais quels bits sont décodés. S17 est un scan
image inexploitable ; S18 ne traite pas la pagination. C'est une **lacune réelle
du corpus primaire**, et il faut l'écrire ainsi.

Ce que disent les sources disponibles, concordantes entre elles :

- S7 (**secondaire**), § « ZX Spectrum +2A / +3 — Memory », littéral : « Port
  0x7ffd behaves in the almost exactly the same way as on the 128K/+2, with two
  exceptions : Bit 4 is now the low bit of the ROM selection. The partial
  decoding used is now slightly different : **the hardware will respond only to
  those port addresses with bit 1 reset, bit 14 set and bit 15 reset** (as
  opposed to just bits 1 and 15 reset on the 128K/+2). » Et pour `&1FFD` :
  « the hardware will respond to all port addresses with bit 1 reset, bit 12 set
  and bits 13, 14 and 15 reset ».
- S12 (**secondaire**) donne les mêmes motifs : `&7FFD` = `01xx xxxx xxxx xx0x`,
  `&1FFD` = `0001 xxxx xxxx xx0x`.
- Fuse (S11, **primaire-pour-le-modèle**), `plus3_memory_ports` :

```c
{ 0xc002, 0x4000, NULL, spec128_memoryport_write },    /* 0x7ffd : A15=0, A14=1, A1=0 */
{ 0xf002, 0x1000, NULL, specplus3_memoryport2_write }, /* 0x1ffd : A15..13=0, A12=1, A1=0 */
```

Ces trois sources donnent exactement la même chose, mais **aucune n'est
primaire**. À traiter comme très bien attesté, non confirmé sur documentation
constructeur.

Sur +2A/+3, les deux plages d'alias ne se recouvrent plus — c'est précisément
l'utilité de l'ajout de A14 au décodage de `&7FFD`. Les ports voisins relèvent de
la même famille de masques : `&2FFD` statut FDC, `&3FFD` données FDC (Fuse,
`upd765_ports`), ce qui recoupe S4 (**primaire**) : « the data register for this
device is at address 3FFDh (16381) and the status register is at 2FFDh (12285) ».
Le verrou imprimante est en `&0FFD` (S4, primaire : « The Centronics parallel
printer port is basically just an 8 bit data latch (74273) whose address is 0FFDh
(4093) »).

#### Écriture seule, et ce que fait une lecture

**Écriture seule, aucune relecture de l'état de pagination possible sur aucun de
ces modèles.** La preuve primaire est indirecte mais nette : le manuel +3 (S3)
prescrit de tenir une copie logicielle, en nommant les variables système — bit 4
de `7FFDh` « (system variable : BANKM) », bit 2 de `1FFDh` « (system variable :
BANK678) ». Les adresses sont **BANKM = `&5B5C`** (23388) et
**BANK678 = `&5B67`** (23399) (S6, primaire ; S7, secondaire). S7 ajoute
l'obligation pratique : « If normal interrupt code is to run, then the system
variable at 0x5b5c (23388) must be kept updated with the last value sent to port
0x7ffd. »

**Sur ce que produit une lecture, les sources se contredisent, et il faut le
dire.**

- S7 (**secondaire**) : « Reading from 0x7ffd produces no special results :
  floating bus values will be returned as would be returned from any other port
  not attached to any hardware. » — lecture inoffensive.
- S13 (**secondaire**), § « HAL bugs », littéral : « **Reads from port 0x7ffd
  cause a crash**, as the 128's HAL10H8 chip does not distinguish between reads
  and writes to this port, resulting in a floating data bus being used to set the
  paging registers. »
- **La source primaire tranche en faveur de S13** : S1/S2 ¶ 4.12.11 dit que BANK
  est décodé de « /IORQ and RD/WR active low (**I/O read or write cycle**) ». Le
  décodage ne distingue donc pas lecture et écriture : une lecture cadence aussi
  le verrou et y capture ce qui traîne sur D0–D5.

**Conclusion, à porter avec sa nuance de modèle** : sur un 128 et sur les
premiers +2 gris, `IN A,(&7FFD)` échantillonne le bus flottant dans le registre
de pagination et plante typiquement la machine. S13 (secondaire, non confirmé)
ajoute que des +2 gris tardifs ont reçu un HAL corrigé sur ce point : « Later grey
+2s were shipped with an updated HAL chip which corrects the issue whereby reads
of port 0x7ffd would crash the machine. » Fuse (S9, S11) modélise le port en
écriture seule avec gestionnaire de lecture `NULL` : il implémente donc la version
inoffensive de S7 et **n'émule pas** le plantage en lecture.

**Pour les +2A/+3, l'effet d'une lecture de `&7FFD` ou `&1FFD` sur la pagination
est non trouvé** : aucune source consultée ne l'aborde. Le seul fait documenté est
négatif : S7 (secondaire) note que sur ces machines « reading from a
non-existing port (eg 0xff) will always return 255, and not give any
screen/attribute bytes as it does on the 48K/128K/+2 » — pas de bus flottant sur
les ports ordinaires.

### A.3 Disposition fixe de l'espace adressable

**Primaire**, S1/S2, § « MEMORY ORGANISATION », diagramme de l'espace adressable
Z80, littéral :

```
C000H           page 0-7
8000H            page 2
4000H            page 5        Screen 1
0000H          ROM 0 or 1
```

et ¶ 4.4 : « Clearly, dependent on register bits B2-B0, the Z80 can access page 2
at address 8000H or C000H and the screen in page 5 at address 4000H or C000H. The
screen in page 7 can only be accessed at address C000H. »

Soit, pour **128 et +2** :

| Fenêtre | Contenu | Source |
|---------|---------|--------|
| `&0000`–`&3FFF` | ROM 0 (ROM 128, éditeur/menu) ou ROM 1 (48 BASIC), selon bit 4 de `&7FFD` | S1/S2 § MEMORY ORGANISATION et ¶ 4.12.2 (primaire) |
| `&4000`–`&7FFF` | **toujours** la banque RAM 5 (écran normal) | S1/S2 § MEMORY ORGANISATION (primaire) |
| `&8000`–`&BFFF` | **toujours** la banque RAM 2 | S1/S2 § MEMORY ORGANISATION (primaire) |
| `&C000`–`&FFFF` | banque 0 à 7 selon bits 0–2 de `&7FFD` | S1/S2 ¶ 4.3 (primaire) |

S7 (secondaire, concordant) insiste sur le point qui compte pour un profil : le
bit 3 change l'écran *affiché*, pas la cartographie — « this does not affect the
memory between 0x4000 and 0x7fff, which is always bank 5 ». Autrement dit,
l'écran shadow (page 7) n'est accessible au Z80 qu'en le paginant en `&C000`.

Le manuel +3 (**primaire**, S4, § Reference — Hardware) dit la même chose pour les
+2A/+3 en mode normal :

> « The four ROM pages (0-3) can be mapped into the bottom 16K (0000h-3FFFh) of
> the memory map. The eight RAM pages (0-7) are usually mapped into the top 16K
> (C000h-FFFFh) of the memory map. **RAM page 5 is also mapped into the range
> 4000h-7FFFh, and RAM page 2 is mapped into the range 8000h-BFFFh.** There are
> also several RAM page combinations that occupy the full 64K address range. »

Fuse implémente exactement cela (S9, S10, `normal_memory_map`) : ROM en `0x0000`,
RAM 5 en `0x4000`, RAM 2 en `0x8000`, banque courante en `0xc000`.

Usage des banques, utile pour savoir lesquelles sont libres — **secondaire, non
confirmé sur primaire** pour le détail :

- 128 / +2 (S7) : « RAM banks 1,3,4,6 and most of 7 are used for the silicon
  disc ; the rest of 7 contains editor scratchpads. »
- +2A / +3 (S7) : « RAM banks 1,3,4 and 6 are used for the disc cache and
  RAMdisc, while Bank 7 contains editor scratchpads and +3DOS workspace. »
  Recoupé côté primaire par S3 : « In BASIC, RAM page 0 is normally in situ. When
  editing or calling +3DOS routines, RAM page 7 is used for various buffers and
  'scratchpads'. »

### A.4 Port `&1FFD` des +2A / +3

#### Champ de bits

**Primaire**, S3 (manuel +3, § Memory management), citation littérale :

> « The +3 also uses I/O port 1FFDh for some ROM and RAM switching. The bit field
> for this address is as follows :
> ```
> D0...D1     - ROM/RAM switching
> D2          - Affects whether D0...D1 work on RAM/ROM
> D3          - Disk motor
> D4          - Parallel port strobe (active low)
> ```
> When bit 0 is 0, bit 1 has no effect and bit 2 is a 'vertical' ROM switch (i.e.
> between ROM 0 and ROM 2, or between ROM 1 and ROM 3). Bit 4 in the port at
> 7FFDh is a 'horizontal' ROM switch (i.e. between ROM 0 and ROM 1, or between
> ROM 2 and ROM 3). »

L'en-tête de champ de bits du manuel est lui-même **mal libellé** : il présente
D0–D1 comme « ROM/RAM switching » et D2 comme le qualificateur, alors que le
texte suivi et les deux tableaux du même chapitre font de D0 le bit de mode et de
D1–D2 les sélecteurs. Le texte suivi et les tableaux sont sans ambiguïté ; c'est
la ligne d'en-tête qui est approximative. S7 (secondaire) formule proprement la
même chose :

| Bit `&1FFD` | Fonction | Source |
|-------------|----------|--------|
| 0 | mode de pagination : **0 = normal, 1 = spécial (all-RAM)** | S3 (primaire) ; S7 (secondaire) |
| 1 | en mode normal : sans effet. En mode spécial : bit 0 du numéro de configuration | S3 (primaire) |
| 2 | en mode normal : **bit de poids fort du numéro de ROM**. En mode spécial : bit 1 du numéro de configuration | S3 (primaire) |
| 3 | **moteur du lecteur de disquette** (« Disk motor ») ; 1 = on, 0 = off | S3 (primaire) pour la fonction ; le sens 1 = on vient de S7 (secondaire, **non confirmé sur primaire**) |
| 4 | **strobe du port parallèle, actif à l'état bas** | S3 et S4 (primaire) |
| 5–7 | **non trouvé** : aucune fonction documentée, et il n'est pas établi qu'ils soient mémorisés | S14 laisse ces bits vides (secondaire) |

S4 (primaire) précise pour le bit 4 : « The STROBE signal for the printer is
produced by the ULA and is accessed using the bit 4 of address 1FFDh (8189). The
state of the BUSY line from the printer is read from bit 0 of address 0FFDh
(4093). » Fuse (S10) se contente de relayer : `printer_parallel_strobe_write( b & 0x10 );`

La raison d'être du mode spécial est donnée par S12 (**secondaire, non
confirmée**) : « This mode introduced, because CP/M cannot operate if ROM mapped
at bank0 (0x0000-0x3fff). »

#### Sélection de ROM sur deux bits

**Primaire**, S3, citation littérale :

> « It is best to think of bit 4 in port 7FFDh and bit 2 in port 1FFDh combining
> to form a 2-bit number (0...3) which determines which ROM occupies the memory
> area 0000h...3FFFh. **Bit 4 of port 7FFDh is the least significant bit and bit
> 2 of 1FFDh is the most significant bit.**
> ```
> Bit 2 of 1FFDh      Bit 4 of 7FFDh      Switched ROM at
> (sys. var.: BANK678)  (sys. var.: BANKM)   0000h...3FFFh
>       0                   0                     0
>       0                   1                     1
>       1                   0                     2
>       1                   1                     3
> ```
>   ROM switching (with Bit 0 of 1FFDh set to 0) »

Identité des quatre ROM, **primaire** : figure « Horizontal and vertical ROM
switching » de S3 — ROM 0 = Editor, ROM 1 = Syntax, ROM 2 = DOS, ROM 3 = 48
BASIC ; et S5 (ch. 8 partie 27, « Guide to +3DOS »), littéral : « The operating
software of the +3 is, in effect, held in four ROMs (though the information is
actually contained in just two ICs). All four ROMs are addressed between 0000h
and 3FFFh, although only one is switched in at a time. ROM 0 is the 'editor' ROM
and is the one entered when the +3 is first switched on… ROM 1 is the 'syntax' ROM
and handles the high level control of +3 BASIC… ROM 3 is the '48 BASIC' ROM and is
virtually identical to the ROM used in the very first Spectrum. » ROM 2 = +3DOS
par élimination dans la figure de S3, et explicitement chez S7 (secondaire).

Fuse (S10) code la recomposition à l'identique :

```c
rom = ( ( ram.last_byte  & 0x10 ) >> 4 )    /* 7FFD bit 4 -> bit 0 du numéro */
    | ( ( ram.last_byte2 & 0x04 ) >> 1 );   /* 1FFD bit 2 -> bit 1 du numéro */
```

#### Les configurations « all-RAM » : il y en a exactement quatre

**Primaire**, S3, citation littérale :

> « When bit 0 of port 1FFDh is set to 1, bits 1 and 2 switch in various RAM
> combinations that occupy the full 64K address space. These are not used by +3
> BASIC but are provided for authors of operating systems/games. When the +3DOS
> 'DOS BOOT' routine is used, the bootstrap is loaded into the 4, 7, 6, 3 RAM
> page environment. The various +3 extra RAM paging options are as follows :
> ```
> Bit 2 of 1FFDh   Bit 1 of 1FFDh   RAM pages used
>                                   (0000h...3FFFh,
>                                    4000h...7FFFh, etc.)
>       0                0            0, 1, 2, 3
>       0                1            4, 5, 6, 7
>       1                0            4, 5, 6, 3
>       1                1            4, 7, 6, 3
> ```
>   Extended memory paging (with Bit 0 of 1FFDh set to 1) »

Table explicite, fenêtre par fenêtre. Le numéro de configuration est
`(&1FFD >> 1) & 3` :

| Config | b2 | b1 | `&0000`–`&3FFF` | `&4000`–`&7FFF` | `&8000`–`&BFFF` | `&C000`–`&FFFF` |
|--------|----|----|-----------------|-----------------|-----------------|-----------------|
| 0 | 0 | 0 | RAM **0** | RAM **1** | RAM **2** | RAM **3** |
| 1 | 0 | 1 | RAM **4** | RAM **5** | RAM **6** | RAM **7** |
| 2 | 1 | 0 | RAM **4** | RAM **5** | RAM **6** | RAM **3** |
| 3 | 1 | 1 | RAM **4** | RAM **7** | RAM **6** | RAM **3** |

Trois sources indépendantes concordent, **sans aucune divergence** : S3
(primaire), S7 et S12 (secondaires), et Fuse (S10, primaire-pour-le-modèle) :

```c
case 0: select_special_map( 0, 1, 2, 3 ); break;
case 1: select_special_map( 4, 5, 6, 7 ); break;
case 2: select_special_map( 4, 5, 6, 3 ); break;
case 3: select_special_map( 4, 7, 6, 3 ); break;
```

avec `which = ( ram.last_byte2 & 0x06 ) >> 1;`

Deux comportements en découlent, tirés de Fuse (**primaire-pour-le-modèle**) et
cohérents avec S3 (« no further paging operations will work ») :

1. **Le verrou (bit 5 de `&7FFD`) verrouille aussi `&1FFD`.**
   `specplus3_memoryport2_write()` commence par
   `if( machine_current->ram.locked ) return;`. À noter pour un profil : le
   `LOCKS` n'est pas l'attribut d'un port, c'est l'attribut de la pagination
   entière.
2. **En mode spécial, les bits 0–2 et 4 de `&7FFD` sont inertes** — les quatre
   fenêtres viennent entièrement de la table ci-dessus — **mais le bit 3 (écran)
   continue de fonctionner**, parce que la page vidéo est décodée directement
   depuis ce bit, indépendamment de la cartographie CPU. S12 (secondaire) le dit
   explicitement : « ULA build the actual picture from RAM page 5+2*V, where
   V = 0 or 1 in page register 0x7ffd (128K) B3, **regardless of actual RAM page
   configuration** ». Fuse calcule bien `screen` avant de brancher entre mode
   normal et mode spécial.

### A.5 Mémoire *contended*

Point essentiel pour un profil : **la contention est un attribut de la banque
RAM, pas de la fenêtre d'adresses.** C'est ce que traduit l'API de Fuse
(`memory_ram_set_16k_contention( page, flag )`, S9/S10), et cela se voit dans les
faits ci-dessous.

#### 48K / 16K

**Secondaire**, S8, § Contended Memory : « When the ULA is drawing the screen, it
needs to access video memory ; the RAM cannot be read by two devices (the ULA and
the processor) at once, and the ULA is given higher priority (as the electron beam
cannot be interrupted), so programs which run in the contended memory (**from
0x4000 to 0x7fff**)… ». S15 concorde. C'est **tout** le bloc de 16 K en `&4000`
qui est ralenti, pas seulement la zone d'affichage. Motif 6,5,4,3,2,1,0,0 à partir
du T-state 14335/14336 après l'interruption, période 224 T-states, 192 lignes
(S7, S8, secondaires).

#### 128 / +2 — banques **1, 3, 5, 7**

**Ici, la source primaire est en contradiction directe avec tout le reste, et
c'est le point le plus important de cette section.**

**S1/S2 ¶ 4.2 (primaire) dit le contraire de la réalité**, littéral :

> « Pages 0-3 are uncontended and are accessed solely by the Z80. **Pages 4-7 are
> contended** in that the Z80 and ULA IC1 both require access to pages 5 and 7 in
> order to generate the memory mapped displays. »

Le diagramme § « MEMORY ORGANISATION » du même manuel groupe « Page 7 / Page 6 /
Page 5 / Page 4 — Contended video RAM (ICs 6-13) » contre « Page 3 / Page 2 /
Page 1 / Page 0 — Uncontended 'upper' RAM (ICs 15-22) », et la table de décodage
du PAL au ¶ 4.6 montre ULA15 = 0 (chemin *contended*) uniquement pour « Page
4/5/6/7 access ».

**Tout le reste dit 1, 3, 5, 7 :**

- S7 (secondaire) : « **Memory banks 1,3,5 and 7 are contended**, which reduces
  the speed of memory access in these banks. »
- S15 (secondaire) : « On the Spectrum 128 and Spectrum +2, memory pages 1, 3, 5
  and 7 are contended. This means that RAM from 0x4000 to 0x7fff is always
  contended (as memory page 5 is always mapped in there) and RAM from 0xc000 to
  0xffff can be contended if page 1, 3, 5 or 7 is paged in there. »
- Fuse (S9, primaire-pour-le-modèle), commentaire et code :

```c
/* Odd pages contended on the 128K/+2; the loop is up to 16 to
   ensure all of the Scorpion's 256Kb RAM is not contended */
for( i = 0; i < 16; i++ )
  memory_ram_set_16k_contention( i, i & 1 ? contention : 0 );
```

- **Note de l'éditeur dans S2** (donc **secondaire**, mais insérée juste après le
  ¶ 4.2 fautif), littérale : « (Editor's note – this is inaccurate. **Pages
  1,3,5 and 7 are contended and pages 0,2,4 and 6 are uncontended. This is due to
  an error in the PAL (IC29) equations. This was not corrected until the arrival
  of the +2A and +3 models.**) »
- S13 (secondaire) : « Due to a bug either in the 128's HAL10H8 chip or in the
  PCB, memory banks 1, 3, 5 and 7 are contended (and the rest uncontended) **as
  opposed to 4, 5, 6 and 7 as documented in the service manual**… The paging
  scheme documented in the manual would have been implemented as was (presumably)
  originally intended **had the B0 and B2 inputs to the HAL10H8 been reversed**. »
- S16 (secondaire) donne la conséquence audible : « an error in the PAL chip which
  resulted in the contended banks being 1,3,5 and 7 instead… This error was
  carried over from the 128 to the grey +2… An example of the net result is
  Fantasy World Dizzy, which contains sampled speech which sounds wrong on a 128
  or grey +2, and correct on a +2A or +3. »

**Résolution.** Le service manual documente l'**intention de conception** ; le
matériel livré a un bug d'équations dans le PAL/HAL10H8 (IC29). Pour le matériel
réel, ce sont bien les banques **1, 3, 5, 7** qui sont ralenties sur 128 et +2
gris. La convergence de six sources indépendantes, dont le modèle de Fuse et une
note d'éditeur qui identifie le composant, l'emporte sur le manuel. Réserve
explicite de S13 (secondaire) : « It is not known whether this also updated the
contention scheme » — on ne sait pas si les +2 gris tardifs à HAL corrigé ont
aussi changé de schéma de contention.

Motif temporel 128/+2 (**secondaire**, S7 et S15 concordants) : même motif
6,5,4,3,2,1,0,0 que le 48K, mais démarrant 14361 T-states après l'interruption au
lieu de 14335, et répété tous les 228 T-states au lieu de 224.

Contention d'entrée-sortie (**secondaire**, S7) : « As on the 48K machine, Port
0xfe (and all other even ports) are contended. **Port 0x7ffd is not contended as
of itself, but the high byte of the port address being 0x7f causes delays.** »

#### +2A / +3 / +2B / +3B — banques **4, 5, 6, 7**, et là primaire et secondaire concordent

**Primaire**, S3, littéral :

> « The RAM banks are of two types : **RAM pages 4 to 7 which are contended**
> (meaning that they share time with the video circuitry), and **RAM pages 0 to 3
> which are uncontended** (where the processor has exclusive use). Any machine
> code which has critical timing loops (such as music or communications programs)
> should keep all such routines in uncontended banks. For example, executing NOPs
> in contended RAM will give an effective clock frequency of 2.66Mhz as opposed to
> the normal 3.55MHz in uncontended RAM. This is a reduction in speed of about
> 25%. »

**Primaire**, S4, littéral : « The RAM is composed of four 16K x 4 bit chips
(41464), some of which (**RAM banks 4-7**) are time-shared between the circuitry
that produces the screen display, and the Z80A. The others (**RAM banks 0-3**) are
for the exclusive use of the Z80A, as in the ROM. For the contended RAM… during
128 out of every 228 CPU T states (1 TV line), and during 192 out of every 311 TV
lines (1 frame) the CPU is allowed only 1 access to contended RAM in every 8 T
states. The CPU is controlled by introducing wait states. »

Fuse (S10) concorde : `/* RAM pages 4, 5, 6 and 7 contended */ for( i = 0; i < 8; i++ ) memory_ram_set_16k_contention( i, i >= 4 );`

**Conséquence sur le mode spécial**, qui découle mécaniquement du fait que la
contention suit la banque : en configurations 1, 2 et 3, la **RAM 4 est en
`&0000`–`&3FFF` et elle est contended** — c'est la seule configuration Spectrum
où le bas de l'espace adressable est ralenti. Et en **configuration 0 (pages 0,
1, 2, 3), rien n'est contended** : les 64 K entiers tournent à pleine vitesse.
Aucune source ne l'énonce verbatim ; c'est une implication directe du modèle par
page de Fuse (S10) et du texte de S4.

Trois particularités +2A/+3 supplémentaires :

1. **Contention appliquée seulement sur /MREQ** — S15 (secondaire, non confirmé
   sur primaire) : « The gate array in the +2A, +3, +2B, and +3B differs more
   significantly in that it applies **less** contention than the ULAs in the
   earlier models. Specifically, it applies memory contention only if the MREQ
   line is active, whereas the 16K/48K ULA applies it under all circumstances. »
2. **Le port `&FE` n'est pas contended sur +2A/+3, et pour `&7FFD` / `&1FFD` on
   ne sait pas** — S7 (secondaire), littéral : « However, Port 0xfe is not ;
   whether ports 0x7ffd and 0x1ffd are contended is currently unknown. »
3. **Pas de « late timing » sur les machines Amstrad** — S15 (secondaire).

#### Contradiction non résolue sur les valeurs absolues de T-states +2A/+3

S7 et S15 (deux secondaires) donnent des tables décalées de 4 T-states : S7 fait
démarrer la table à 14365, S15 à 14361. Aucune source primaire ne donne de valeur
absolue — S4 ne donne que les rapports (128 T-states sur 228, 192 lignes sur 311,
un accès par 8 T-states). **Question ouverte** : à calibrer empiriquement si le
timing exact importe.

### A.6 Divergence sur la numérotation des écrans (sans effet sur les bits)

Le manuel +3 (S3, **primaire**) écrit « D3 switches screens : **screen 0 is held
in RAM 7** (beginning at C000h) », alors que S1/S2 (**primaire** aussi) étiquette
« Page 7 — screen 2 » et « Page 5 — screen 1 », et S7 (secondaire) parle de
normal (0) = banque 5 et shadow (1) = banque 7.

**La sémantique du bit n'est contestée par aucune source** : `&7FFD` bit 3 = 0 →
l'ULA affiche la RAM 5 ; = 1 → l'ULA affiche la RAM 7. C'est confirmé au niveau
matériel par S1/S2 § 5.3.3 (table de décodage VB → DMA7). Seule l'**étiquette**
« screen 0 / 1 / 2 » varie d'un manuel à l'autre, et la phrase de S3 est un
erratum de numérotation. Pour un profil, n'utiliser que les numéros de banque, 5
et 7, jamais « screen 0 ».

---

## B. MSX

### B.1 Slots, sous-slots, pages

**Quatre slots primaires, extensibles en quatre sous-slots chacun, 16 slots au
maximum, 1 Mo — primaire.** M2, chapitre 5 § 7.1.1 « Basic slot and expansion
slot » (<https://konamiman.github.io/MSX2-Technical-Handbook/md/Chapter5b.html>) :

> « The standard MSX machine can have up to four basic slots. The basic slot can
> be expanded up to four slots by connecting a slot expansion box … and is called
> 'expansion slots'. When each of four basic slots is expanded to four expansion
> slots, the maximum number of slots is 16. If you multiply 16 slots x 64K bytes
> you will get 1M bytes of accessible address space. »

Le même passage précise qu'il n'y a **pas de troisième niveau d'imbrication** :
« the system itself cannot be started when expansion slot boxes are connected to
the expansion slot ». Et M1 § 1.6.2, p. 36 (primaire) : « The slots directly
attached to the MSX computer itself must be primary slots. »

**Quatre pages de 16 K, et une page ne peut pas changer de numéro — primaire.**
M2 § 7.1.1 : « Each slot has 64K bytes from 0000H to FFFFH of address space and
MSX manages it by dividing it into four 'pages' of 16K bytes each », puis
« **Note that a page with a given page number cannot be assigned to a page with a
different page number (that is, page n of each slot is also page n to the CPU).** »
M1 § 1.7.1, p. 38 le redit : « The physical memory is always allocated to the same
memory page in the CPU memory space. It is not possible to allocate it to a
different page, as in allocating page 3 of slot 3 to page 0 of the CPU memory
space. » Définition de terminologie, M1 § 2.2.3, p. 161 : « Page : Memory block
(maximum 16K) in each slot. The slots are divided into four pages (0000H to
3FFFH, 4000H to 7FFFH, 8000H to 0BFFFH, and 0C000H to 0FFFFH). »

C'est une contrainte structurante pour un profil : la fenêtre et la page sont la
même chose sur MSX, il n'y a pas de liberté de placement.

### B.2 Sélection de slot primaire : le PPI, port `&A8`

**Le port `&A8` est bien le port A du PPI i8255, et c'est bien lui le registre de
sélection de slot — primaire, trois attestations indépendantes :**

- M1 § 1.7.6 « PPI Port », p. 41 : `A8H R/W Port A` / `A9H R/W Port B` /
  `AAH R/W Port C` / `ABH R/W Mode register`.
- M1 § 1.7.1, p. 38 : « **The slot select register, port A of the 8255**, maps the
  physical memory space to the logical CPU memory space in 16K-byte units
  (pages). »
- M1 § 1.6.2, p. 36 : « Primary slots are those slots managed by the slot select
  register provided in **port A of the 8255**. »
- M2, annexe 6 (I/O MAP,
  <https://konamiman.github.io/MSX2-Technical-Handbook/md/Appendix6.html>) :
  « **A8H to ABH** : parallel port (8255) — A8H : port A ; A9H : port B ;
  AAH : port C ; ABH : mode set ».

**Disposition des bits : deux bits par page, page 0 dans les bits de poids
faible — primaire, avec table et exemple chiffré.** M1 § 1.7.11 « 8255 (PPI) Bit
Assignments », p. 43, table du port A en direction OUTPUT :

| Port | Bits | Signaux | Description |
|------|------|---------|-------------|
| A | 0, 1 | CS0L, CS0H | 0000-3FFF Address slot select signal |
| A | 2, 3 | CS1L, CS1H | 4000-7FFF Address slot select signal |
| A | 4, 5 | CS2L, CS2H | 8000-BFFF Address slot select signal |
| A | 6, 7 | CS3L, CS3H | C000-FFFF Address slot select signal |

M1 § 1.7.1, p. 38 donne le diagramme avec la valeur explicite `00 10 00 00` :

```
(MSB) 7 6  5 4  3 2  1 0 (LSB)
     | 0 0 | 1 0 | 0 0 | 0 0 |
        |     |     |     +--- Allocate slot 0 for page 0
        |     |     +--------- Allocate slot 0 for page 1
        |     +--------------- Allocate slot 2 for page 2
        +--------------------- Allocate slot 0 for page 3
```

M2 § 7.1.2, figure 5.38 « Selecting the basic slot » (primaire) est identique :
bits 0-1 → « Basic slot number of page 0 (0 to 3) », 2-3 → page 1, 4-5 → page 2,
6-7 → page 3. M5 (secondaire) concorde exactement. **Aucune divergence entre
sources.** Page 0 en bas, page 3 en haut.

**Le port est relisible — primaire, avec une nuance.** M1 § 1.7.6, p. 41 marque
`A8H R/W`. Le § 1.7.11 déclare le port A en sortie ; la relecture fonctionne
parce que le 8255 en mode 0 mémorise un port de sortie et rend le contenu du
verrou. openMSX (M4, primaire-pour-le-modèle) implémente exactement cela,
`src/I8255.cc`, `I8255::readPortA()` :

```cpp
case MODEA_0:
    if (control & DIRECTION_A) {
        return interface.readA(time);   // input not latched
    } else {
        return latchPortA;              // output is latched
    }
```

et `src/MSXPPI.cc` route l'écriture vers la logique de slots :
`void MSXPPI::writeA(uint8_t value, ...) { getCPUInterface().setPrimarySlots(value); }`.

Confirmation au niveau BIOS (**primaire**, M2 annexe 1) : `RSLREG (0138H)`
« reads the contents of current output to the basic slot register » ;
`WSLREG (013BH)` « writes to the primary slot register ». La formule « current
output » donne la nuance exacte : on relit la dernière valeur écrite, pas un état
du bus.

Réserve à signaler : openMSX a un `MSXPPI::peekA()` qui renvoie 0 avec le
commentaire « port A is normally an output on MSX, reading from an output port is
handled internally in the 8255 / TODO check this on a real MSX ». Ce chemin ne
concerne que le cas où le logiciel programme fautivement le port A en entrée ;
il ne contredit pas le `A8H R/W` de M1.

Le décodage d'openMSX (M4, `MSXCPUInterface::setPrimarySlots()`) confirme l'ordre
des bits et montre que l'état de sous-slot est réévalué par page :

```cpp
if (uint8_t ps0 = (value >> 0) & 3; primarySlotState[0] != ps0) { ... uint8_t ss0 = (subSlotRegister[ps0] >> 0) & 3; ... }
if (uint8_t ps1 = (value >> 2) & 3; ...) { ... (subSlotRegister[ps1] >> 2) & 3 ... }
```

### B.3 Sélection des sous-slots : le registre `&FFFF`

**Le registre est bien mappé en mémoire à `&FFFF` — primaire.** M2 § 7.1.2 :
« for expansion slots, it is done through the **'expansion slot selection
register (FFFFH)'** of the installed expansion slot ». M1 § 1.6.2, p. 36 : « The
location of the slot select register for the additional slots is **address FFFF of
the primary slot**. » M1 § 2.2.3, p. 161, terminologie : « Secondary slot : Slot
enabled by the expansion slot register at 0FFFFH. » La figure 1.7 de M2
(chapitre 1 § 2.1.1) montre `FFFFH` étiqueté « Slot Selection Register » dans la
carte mémoire de la MAIN-RAM.

**Disposition des bits : deux bits par page, même ordre que `&A8` — primaire.**
M2 § 7.1.2, figure 5.39 « Selecting the expansion slot » : bits 0-1 →
« Expn. slot number of page 0 (0 to 3) », 2-3 → page 1, 4-5 → page 2, 6-7 →
page 3. M5 (secondaire) concorde, en associant les paires aux plages
`#0000-#3FFF` / `#4000-#7FFF` / `#8000-#BFFF` / `#C000-#FFFF`.

**La particularité de lecture est réelle, c'est un complément à un (NOT
bit-à-bit), et elle est volontaire — primaire, citation littérale.** M1 § 1.6.2
« Slot Expansion », p. 36 :

> « The location of the slot select register for the additional slots is address
> FFFF of the primary slot. **To make it possible to differentiate the register
> from ordinary RAM, take the complement of the output of the register. That is,
> when the register is read, the data is the complement of the value of the
> register.** »

M2 § 7.1.2 (note sous la figure 5.39) dit la même chose en termes plus flous :
« **to identify the kind of slot, in the case of the expansion slot, the value
written is read as the reversed value.** The value of this register is the same
inside the basic slot. » — cette dernière phrase donne au passage un fait
important : les quatre sous-slots d'un même slot primaire voient le même registre.

openMSX (M4, primaire-pour-le-modèle) implémente exactement `0xFF ^ value`,
`src/cpu/MSXCPUInterface.cc`, `readMemSlow()` et `peekMem()` :

```cpp
if ((address == 0xFFFF) && isExpanded(primarySlotState[3])) [[unlikely]] {
    return 0xFF ^ subSlotRegister[primarySlotState[3]];
}
```

M1 § 1.5.5, p. 35 donne même le circuit de référence : un verrou 74LS273 dont les
sorties reviennent sur le bus de données à travers un inverseur, validé par
`(accès à FFFF)·RD·SLTSEL` et cadencé par `(accès à FFFF)·WR·SLTSEL`, avec
A14/A15 attaquant un 74LS153/74LS139 pour produire le SLTSL étendu par page.
C'est l'explication physique à la fois du complément et du décodage en paires de
bits.

**Divergence de vocabulaire à noter** : M1 dit « complement », la traduction de
M2 dit « reversed value ». Le `0xFF ^ value` d'openMSX tranche : il s'agit du
**complément à un**, pas d'une inversion de l'ordre des bits. Le « reversed » de
M2 est un rendu approximatif du même fait.

**Un registre `&FFFF` par slot primaire étendu, et c'est celui du slot primaire
sélectionné en page 3 qui répond — confirmé.** openMSX tient
`subSlotRegister[4]` (un par slot primaire) et indexe lecture comme écriture par
`primarySlotState[3]`. Le chemin d'écriture porte une note vérifiée sur machine
réelle :

```cpp
if ((address == 0xFFFF) && isExpanded(primarySlotState[3])) [[unlikely]] {
    setSubSlot(primarySlotState[3], value);
    // Confirmed on turboR GT machine: write does _not_ also go to
    // the underlying (hidden) device. But it's theoretically
    // possible other slot-expanders behave different.
}
```

M5 (**secondaire**) en tire la conséquence pratique, qui est exactement le genre
de chose qu'un profil doit porter : « **the subslot register only affects the
primary slot which is selected in page 3** (which the #FFFF address is in). This
makes selecting a different subslot in e.g. page 1 a little less than trivial when
page 1 doesn't have the same primary slot as page 3. This involves switching
page 1's slot into page 3, selecting the desired subslot, and switching back the
original slot into page 3. All this must be done with interrupts disabled and,
logically, from an address not in the #C000-#FFFF range. Don't forget to take
care of the stack as well. »

Format de l'octet d'identification de slot (**primaire**, M2 § 7.1.2 figure 5.40,
et entrée BIOS `RDSLT` de M1) : `F000EEPP` — bits 0-1 slot primaire, bits 2-3
slot d'extension (secondaire), bits 4-6 inutilisés, bit 7 = 1 si slot étendu.
Miroirs BIOS (**primaire**, M2 annexe 4) : `EXPTBL (FCC1H, 4)` = « flag table for
expansion slot ; whether the slot is expanded » ; `SLTTBL (FCC5H, 4)` = « current
slot selection status for each expansion slot register ». Cette dernière confirme
qu'il y a exactement quatre registres de ce type, un par slot primaire, et que le
BIOS en tient une copie logicielle parce qu'ils ne sont pas relisibles tels quels.

### B.4 Memory mapper RAM

**Ports `&FC`–`&FF`, un par page, dans l'ordre des pages.** La plage est
**primaire** : M2 annexe 6 (I/O MAP) — « **FCH to FFH : memory mapper** ». C'est
tout ce que dit la carte officielle : elle donne la plage, pas l'affectation port
par port.

L'affectation port ↔ page ne vient que de **M5 (secondaire)** :

| Port | Description |
|------|-------------|
| `#FC` (écriture) | segment pour la page 0 (`#0000`-`#3FFF`) |
| `#FD` (écriture) | segment pour la page 1 (`#4000`-`#7FFF`) |
| `#FE` (écriture) | segment pour la page 2 (`#8000`-`#BFFF`) |
| `#FF` (écriture) | segment pour la page 3 (`#C000`-`#FFFF`) |

openMSX (M4, primaire-pour-le-modèle) confirme le même ordre :
`MSXMotherBoard::createMapperIO()` fait
`register_IO_InOut_range(0xfc, 4, mapperIO.get())`, et `MSXMemoryMapper::writeIO()`
fait `byte page = port & 3; … fillDeviceRWCache(page * 0x4000, 0x4000, data);`.
Donc `&FC` pilote la page 0 du Z80 et `&FF` la page 3.

**Aucun texte primaire ne donne cette affectation port par port.** À traiter comme
très bien attesté (M5 et openMSX concordants), non confirmé sur documentation
constructeur.

**Granularité de banque : 16 K — confirmé.** openMSX,
`MSXMemoryMapperBase` : `getBaseSizeAlignment() { return 0x4000; }`,
`calcAddress()` = `segmentOffset(address / 0x4000) | (address & 0x3fff)`,
`segmentOffset()` = `segment * 0x4000`, et un contrôle de configuration qui lève
« Mapper size is not a multiple of 16K ». M5 (secondaire) : « A memory mapper
divides the 64k of RAM into four 16k blocks called pages, into which up to 256
different memory segments can be mapped. Note that these segments are shared — it
is possible to map a segment used in page 0 into page 1 as well. »

**Nombre de bits de segment : nominalement 8 (256 segments = 4 Mo), mais seuls les
bits réellement implémentés se relisent.** M5 (secondaire) : « up to 256 different
memory segments ». openMSX impose `kSize > 4096` → « Mapper size must not be
larger than 4096kB », soit 256 × 16 K, et masque les écritures à la largeur
implémentée : `registers[port & 3] = value & byte(std::bit_ceil(numSegments) - 1)`.

**Relisibilité des ports : les sources ne sont contradictoires qu'en apparence,
et c'est la source primaire qui les réconcilie.**

- **primaire** — M3, § 5.7 « Direct paging routines », p. 30, littéral :
  « The 'GET' routines return values from internal images of the registers without
  actually reading the registers themselves. This ensures that if a segment is
  enabled by, for example, 'PUT_P1' then a subsequent 'GET_P1' call will return
  the actual value. **Reading the mapper register may produce a different value
  because the top bits of the segment numbers are generally not recorded.** »
- **secondaire** — M5 : « Note that **reading those registers is not reliable,
  and should not be done.** » (M5 ajoute que sous MSX-DOS 2 il ne faut pas
  accéder directement aux registres de mapper.)
- openMSX (primaire-pour-le-modèle) implémente bien la lecture :
  `MSXMemoryMapperBase::peekIO()` renvoie
  `registers[port & 3] | byte(~(std::bit_ceil(numSegments) - 1))` — les bits
  implémentés se relisent correctement, les bits non implémentés remontent à 1. Et
  `MSXMapperIO` porte une valeur de configuration `MapperReadBackBits` par
  machine (« largest » = 8 bits relisibles, ou une valeur explicite 0–8), parce
  que les machines réelles diffèrent.

**Synthèse** : les ports *sont* physiquement relisibles sur pratiquement toutes
les machines, mais seulement pour autant de bits que ce mapper-là implémente
réellement ; le reste remonte à 1. C'est exactement pour cela qu'ASCII écrit
« may produce a different value » et que M5 écrit « not reliable ». La largeur du
registre est de 8 bits dans le standard ; la largeur *implémentée* dépend de la
machine. Un profil qui voudrait relire l'état de pagination ne peut pas s'y fier.

Deux faits primaires supplémentaires sur la page 3, M3 § 5.7, p. 30 : « Although a
'PUT_P3' routine is provided, it is in fact a dummy routine and will not alter the
page-3 register. This is because the contents of the page-3 register should never
be altered. » Et M3 § 2.5 « RAM PAGING », p. 9 : « A program … must never alter
page-3 (nothing is allowed to do that !). »

Fait connexe (**secondaire**, M5) : les ports `#F8`–`#FB` portent le « Memory
Mapper segment selection MSB in 16-bit mappers (PlaySoniq, disabled by default) »
— extension non standard, hors norme MSX.

État après reset (openMSX, primaire-pour-le-modèle),
`MSXMemoryMapperBase::reset()` : « Most mappers initialize to segment 0 for all
pages. On MSX2 and higher, the BIOS will select segments 3..0 for pages 0..3. »

### B.5 Mappers de mega-ROM

**Mise en garde sur la qualité des sources, énoncée d'entrée.** Aucune source
primaire (ASCII, Konami, Sony) spécifiant Konami4, Konami5/SCC, ASCII8 ou ASCII16
n'a été trouvée. Ce sont des mappers de cartouche tiers qui n'ont jamais fait
partie du standard MSX, et ni le MSX Technical Data Book ni le MSX2 Technical
Handbook ne les mentionnent : une recherche sur « mapper » / « megarom » dans le
markdown complet du Technical Handbook ne donne rien sur les mappers de cartouche
(le § 7.3 du chapitre 5 ne traite que de l'**en-tête** de cartouche `AB` / INIT /
STATEMENT / DEVICE / TEXT). Les meilleures sources disponibles sont donc :

- **openMSX** (M4), primaire-pour-le-modèle ;
- **M6** (BiFi / Sean Young), **secondaire**, mais qui indique pour chaque entrée
  que la valeur a été vérifiée sur la cartouche physique (« This has been verified
  on… »).

**Les deux concordent sur toutes les valeurs ci-dessous, sans un seul écart.** Les
chiffres sont donc solides, mais **issus de reverse engineering, non spécifiés
officiellement** — cela doit être écrit dans le profil.

Le MSX Datapack vol. 3, non lu (scan japonais sans OCR), contient peut-être une
spécification ASCII8/ASCII16, puisque c'est ASCII qui a fait ces puces. Cela n'est
ni confirmé ni infirmé ici.

#### Point crucial : les quatre commutent par ÉCRITURE MÉMOIRE, pas par port — CONFIRMÉ

M6 (secondaire), introduction : « These mappers divide the memory area 4000h -
BFFFh in two or four memory areas (banks) … **You can write to a certain address
or address area (if it is a memory area, any address in the area will do the same
thing).** 0 selects the first 8Kb or 16Kb of the real ROM into a memory bank, 1
the second, etc.. By default, bank 1 has the value 0 and bank 2 the value 1, etc. »

openMSX (M4) : les quatre mappers implémentent
`writeMem(uint16_t address, byte value, …)` et **aucun** n'implémente `writeIO`.
`RomKonami`, `RomKonamiSCC`, `RomAscii8kB` et `RomAscii16kB` sont des `MSXDevice`
mémoire avec surcharge de `getWriteCacheLine()`. Il n'y a aucun
`register_IO_Out` dans ces fichiers.

**Réfuté : aucun de ces quatre mappers n'utilise un port d'entrée-sortie.** Par
contraste, le memory mapper RAM du § B.4 *est* en entrée-sortie. C'est cette
asymétrie qui justifie de disposer à la fois d'un `OUT` et d'un `POKE` dans le
vocabulaire de `SELECT`.

#### Konami sans SCC (« Konami4 », `RomKonami` chez openMSX)

Banque de **8 K** ; quatre fenêtres `4000`-`5FFF` / `6000`-`7FFF` /
`8000`-`9FFF` / `A000`-`BFFF`.

openMSX, `src/memory/RomKonami.cc`, commentaire d'en-tête
(primaire-pour-le-modèle) :

> « **page at 4000 is fixed**, other banks are switched by writing at 0x6000,
> 0x8000 and 0xA000 (those addresses are used by the games, but **any other
> address in a page switches that page as well**) »

et l'implémentation :

```cpp
void RomKonami::writeMem(uint16_t address, byte value, EmuTime)
{
    // Note: [0x4000..0x6000) is fixed at segment 0.
    if (0x6000 <= address && address < 0xC000) {
        bankSwitch(address >> 13, value);
    }
}
```

| Fenêtre | Plage de commutation | Adresse utilisée par les jeux |
|---------|----------------------|-------------------------------|
| `4000`-`5FFF` | **aucune — fixée au segment 0** | — |
| `6000`-`7FFF` | `6000h`-`7FFFh` (8 K entiers) | `6000h` |
| `8000`-`9FFF` | `8000h`-`9FFFh` (8 K entiers) | `8000h` |
| `A000`-`BFFF` | `A000h`-`BFFFh` (8 K entiers) | `A000h` |

M6 (secondaire) donne exactement la même table, avec « Bank 1 : `<none>` », et
**confirme la banque 0 fixe par une preuve comportementale** : « Note that in
order for The Game Master 2 to work with a cartridge of this type, **bank 1 must
be fixed at 0**. This is the reason The Game Master 2 doesn't work with these
cartridges on fMSX. While The Game Master 2 searches for memory in page 1 and 2 it
overwrites 4000h. If bank 1 is changed, it won't find the AB code and the
cartridge won't be detected. »

**La banque fixe en `&4000` est donc bien réelle**, attestée indépendamment par
openMSX et par les essais sur cartouche de M6. L'entrée « Hai no Majutsushi » de
M6 le redit : « Bank 1 is always fixed to the first block (0). The others can be
changed by writing to **any** address in their memory area. This is important for
emulators because the game uses A000h **and B000h** to switch the last bank. »

Deux détails openMSX supplémentaires : le mapper fait **256 Ko quelle que soit la
taille de la ROM** — `setBlockMask(31)` avec le commentaire « Konami mapper is
256kB in size, even if ROM is smaller », et un avertissement au-delà de 256 Ko
(« not supported on real Konami mapper chips ») ; M6 concorde : « Although the ROM
can be smaller than 256Kb the mapper will remain that size all the time. The empty
area will always be entirely filled with FFh. » Le miroitage est spécifique :
`[0x4000-0x8000)` miroité dans `[0x0000-0x4000)`, `[0x8000-0xC000)` miroité dans
`[0xC000-0x10000)`, avec le commentaire « **Note : the mirror behavior is
different from RomKonamiSCC !** ». État après reset : banques 0, 1, 2, 3 dans les
quatre fenêtres.

#### Konami avec SCC (« Konami5 », `RomKonamiSCC`)

Banque de **8 K** ; les mêmes quatre fenêtres de 8 K. La commutation se fait par
écriture dans une **sous-plage de 2 K** de chaque bloc de 8 K, et non dans le bloc
entier.

openMSX, `src/memory/RomKonamiSCC.cc`, commentaire d'en-tête :

> « The address to change banks : bank 1 : 0x5000 - 0x57ff (0x5000 used) ;
> bank 2 : 0x7000 - 0x77ff (0x7000 used) ; bank 3 : 0x9000 - 0x97ff (0x9000
> used) ; bank 4 : 0xB000 - 0xB7ff (0xB000 used) »

implémentation :

```cpp
if ((address & 0x1800) == 0x1000) {
    // page selection
    auto region = address >> 13;
    bankSwitch(region, value);
}
```

à l'intérieur d'un garde `if ((address < 0x5000) || (address >= 0xC000)) return;`.
Les bits d'adresse 12-11 doivent valoir `10`, ce qui sélectionne exactement
`x000h`-`x7FFh` dans chaque bloc de 8 K.

| Fenêtre | Plage de commutation | Adresse utilisée |
|---------|----------------------|------------------|
| `4000`-`5FFF` | `5000h`-`57FFh` | `5000h` |
| `6000`-`7FFF` | `7000h`-`77FFh` | `7000h` |
| `8000`-`9FFF` | `9000h`-`97FFh` | `9000h` |
| `A000`-`BFFF` | `B000h`-`B7FFh` | `B000h` |

**Les quatre fenêtres ont un registre ici** — contrairement à Konami4, la fenêtre
`4000`-`5FFF` *est* commutable. M6 donne la table identique.

Activation du SCC et fenêtre SCC (openMSX et M6 concordants) : écrire en
`9000h`-`97FFh` une valeur dont les bits 0-5 sont tous à 1 mappe le SCC en
`9800h`-`9FFFh`. openMSX :
`if ((address & 0xF800) == 0x9000) { bool newSccEnabled = ((value & 0x3F) == 0x3F); … }`.
M6 : « writing a value with bits 0 - 5 set (3Fh, bits 6 and 7 do not matter) to
9000h - 97FFh ». **À noter, le recouvrement : `9000h`-`97FFh` est à la fois le
registre de la banque 3 et le registre d'activation du SCC — une seule écriture
fait les deux**, et openMSX enchaîne effectivement les deux traitements sur la
même écriture.

Miroitage (openMSX, différent de Konami4) : `[0x4000-0x8000)` miroité dans
`[0xC000-0x10000)`, `[0x8000-0xC000)` miroité dans `[0x0000-0x4000)`. Limite de
512 Ko avec avertissement au-delà (« not supported on real Konami SCC mapper
chips ») ; M6 : « Unlike the Konami without SCC one this mapper does repeat the
ROM just after the last ROM page. »

#### ASCII 8 K (`RomAscii8kB`)

Banque de **8 K** ; quatre fenêtres `4000`-`5FFF` / `6000`-`7FFF` /
`8000`-`9FFF` / `A000`-`BFFF`. **Les quatre registres sont tous dans la moitié
haute de la page 1**, `6000h`-`7FFFh`, par pas de 2 K.

openMSX, `src/memory/RomAscii8kB.cc`, commentaire d'en-tête :

> « The address to change banks : bank 1 : 0x6000 - 0x67ff (0x6000 used) ;
> bank 2 : 0x6800 - 0x6fff (0x6800 used) ; bank 3 : 0x7000 - 0x77ff (0x7000
> used) ; bank 4 : 0x7800 - 0x7fff (0x7800 used) »

```cpp
if ((0x6000 <= address) && (address < 0x8000)) {
    auto reg = (address >> 11) & 3;
    byte region = reversedRegs ? byte(5 - reg) : byte(reg + 2);
    setRom(region, value);
}
```

| Plage de commutation | Fenêtre affectée |
|----------------------|------------------|
| `6000h`-`67FFh` | `4000`-`5FFF` |
| `6800h`-`6FFFh` | `6000`-`7FFF` |
| `7000h`-`77FFh` | `8000`-`9FFF` |
| `7800h`-`7FFFh` | `A000`-`BFFF` |

M6 donne la table identique (« This has been verified on Valis (Fantasm
Soldier) »). Reset : les quatre fenêtres au segment 0 ; pages 0 et 3 non mappées.

openMSX documente une variante à connaître : avec `reversedRegs`, les mêmes quatre
registres sont câblés dans l'ordre inverse (`6000h`-`67FFh` → `A000`-`BFFF`, …,
`7800h`-`7FFFh` → `4000`-`5FFF`), « Used by the retail cartridge of XeGrader ».
Ce n'est pas une contradiction : c'est une variante matérielle distincte.

#### ASCII 16 K (`RomAscii16kB`)

Banque de **16 K** ; deux fenêtres `4000`-`7FFF` et `8000`-`BFFF`.

openMSX, `src/memory/RomAscii16kB.cc`, commentaire d'en-tête :

> « The address to change banks : first 16kb : 0x6000 - 0x67ff (0x6000 used) ;
> second 16kb : 0x7000 - 0x77ff (0x7000 and 0x77ff used) »

```cpp
if ((0x6000 <= address) && (address < 0x7800) && !(address & 0x0800)) {
    byte region = ((address >> 12) & 1) + 1;
    setRom(region, value);
}
```

| Plage de commutation | Fenêtre affectée |
|----------------------|------------------|
| `6000h`-`67FFh` | `4000`-`7FFF` |
| `7000h`-`77FFh` | `8000`-`BFFF` |

Le terme `!(address & 0x0800)` signifie que `6800h`-`6FFFh` et `7800h`-`7FFFh`
**ne commutent rien** sur ASCII16 — précisément les plages qui *commutent* sur
ASCII8. M6 concorde exactement, y compris le « (7000h and 77FFh used) » et le
« This has been verified on Xevious ». Reset : page 1 → segment 0, page 2 →
segment 0, pages 0 et 3 non mappées. M6 note une exception : « Gallforce is a
special case. It is the same as Xevious, but bank 2 has to start with the first
16kB after a reset. »

#### Récapitulatif

| Mapper | Taille de banque | Fenêtres | Mécanisme | Adresses de commutation | Banque fixe ? |
|--------|------------------|----------|-----------|-------------------------|---------------|
| Konami4 (sans SCC) | 8 K | 4 (`4000`/`6000`/`8000`/`A000`) | **écriture mémoire** | `6000`-`7FFF`, `8000`-`9FFF`, `A000`-`BFFF` (blocs de 8 K entiers) | **oui, `4000`-`5FFF` fixée au segment 0** |
| Konami5 (SCC) | 8 K | 4 | **écriture mémoire** | `5000`-`57FF`, `7000`-`77FF`, `9000`-`97FF`, `B000`-`B7FF` | non |
| ASCII8 | 8 K | 4 | **écriture mémoire** | `6000`-`67FF`, `6800`-`6FFF`, `7000`-`77FF`, `7800`-`7FFF` | non |
| ASCII16 | 16 K | 2 (`4000`/`8000`) | **écriture mémoire** | `6000`-`67FF`, `7000`-`77FF` | non |

### B.6 Conséquence : une écriture de donnée ordinaire commute-t-elle par accident ?

**Oui, sans ambiguïté, et les deux sources le disent en propres termes.**

M6 (secondaire), introduction : « You can write to a certain address or address
area (**if it is a memory area, any address in the area will do the same
thing**). »

openMSX, `RomKonami.cc`, commentaire d'en-tête (primaire-pour-le-modèle) : les
trois adresses de commutation « are used by the games, but **any other address in
a page switches that page as well** ».

L'entrée « Hai no Majutsushi (Mah Jong 2) » de M6 en donne la preuve pratique :
« The others can be changed by writing to **any** address in their memory area.
**This is important for emulators** because the game uses A000h **and** B000h to
switch the last bank. » Autrement dit, un émulateur qui ne décoderait que
l'adresse canonique (`A000h`) casse un jeu commercial réel. C'est une preuve
directe que le décodage partiel est la vérité du matériel, non une commodité
d'implémentation.

**Mais « tout le bloc de 8 K » contre « seulement certaines adresses » dépend du
mapper, et c'est là le nœud pratique pour le linker :**

- **Konami4 : le bloc de 8 K entier.**
  `if (0x6000 <= address && address < 0xC000) bankSwitch(address >> 13, value);`
  — **toute** écriture n'importe où entre `6000h` et `BFFFh` commute une banque.
  Il n'existe aucune zone inscriptible en données dans la fenêtre propre de la
  cartouche. C'est le pire cas : un `LD (HL),A` égaré avec HL n'importe où dans
  24 Ko corrompt la cartographie.
- **Konami5/SCC : seulement les sous-plages de 2 K** `x000h`-`x7FFh` de chaque
  bloc de 8 K (`(address & 0x1800) == 0x1000`). Les écritures en `x800h`-`xFFFh`
  sont ignorées — sauf `9800h`-`9FFFh` quand le SCC est activé, où elles vont au
  SCC. La moitié de chaque bloc de 8 K est donc sûre.
- **ASCII8 : seulement `6000h`-`7FFFh`**, mais ces 8 Ko entiers (en quatre bandes
  de registres de 2 K). Les écritures ailleurs dans `4000h`-`BFFFh` ne font rien.
- **ASCII16 : seulement `6000h`-`67FFh` et `7000h`-`77FFh`.** Les écritures en
  `6800h`-`6FFFh` et `7800h`-`7FFFh` sont ignorées.

Les surcharges `getWriteCacheLine()` d'openMSX encodent exactement ces plages de
décodage : elles renvoient `nullptr` (« il faut piéger toute écriture ici ») pour
les plages de commutation et `unmappedWrite.data()` ailleurs. Pour Konami4 :

```cpp
byte* RomKonami::getWriteCacheLine(uint16_t address)
{
    return (0x6000 <= address && address < 0xC000) ? nullptr : unmappedWrite.data();
}
```

C'est l'énoncé vérifiable par machine de « toute écriture entre `6000h` et
`BFFFh` est une commutation ».

**Ce que la documentation officielle dit là-dessus : rien de direct** — elle ne
décrit jamais ces mappers. Le MSX Technical Data Book documente en revanche le
principe des registres mappés en mémoire, § 1.6.3 « I/O Expansion », p. 37
(primaire) : les périphériques d'entrée-sortie « should be placed in the memory
area because they will be managed by slot select logic and the memory cannot be
accessed simultaneously when placed in different slots » ; et § 1.7.10, p. 42 :
« Addresses 00 to 3F are free. Different devices using the same address must not
be accessed simultaneously. In general, the I/O devices that are not defined here
should be placed in the memory space as memory-mapped I/O. » L'**architecture**
sanctionne donc les registres de cartouche mappés en mémoire ; le décodage partiel
d'adresse — et donc le risque de commutation accidentelle — est un choix
d'économie des fabricants de cartouches, que le standard ne bénit ni n'interdit.

Un dernier artefact d'openMSX, pertinent ici : `MSXCPUInterface::writeMemSlow()`
porte un mécanisme `globalWrites` commenté « **slot-select-ignore writes (Super
Lode Runner)** ». Certaines cartouches espionnent les écritures indépendamment du
slot sélectionné : sur matériel réel, le rayon d'action d'une écriture égarée peut
donc même franchir les slots pour certains mappers.

---

## C. Contradictions et lacunes, récapitulées

### ZX Spectrum

| # | Sujet | Source A | Source B | Appréciation |
|---|-------|----------|----------|--------------|
| C1 | **Banques contended sur 128/+2** | **S1/S2 ¶ 4.2 et ¶ 4.6 (primaire)** : pages **4-7** contended | **S7, S15, S9 (Fuse), S13, S16, note d'éditeur de S2** : pages **1, 3, 5, 7** | **B a raison pour le matériel livré.** A documente l'intention ; une erreur d'équations du PAL/HAL10H8 (IC29) l'a inversée. Corrigé seulement sur +2A/+3. C'est la divergence la plus importante du domaine. |
| C2 | **Effet d'une lecture de `&7FFD` sur 128/+2** | **S7 (secondaire)** : inoffensif, bus flottant | **S13 (secondaire)** : plantage — appuyé par **S1/S2 ¶ 4.12.11 (primaire)** : décodage sur « I/O read or write cycle » | **B a raison**, et le primaire est derrière. S7 est inexact ici. Corrigé sur les +2 gris tardifs (S13, non confirmé). Fuse implémente la version inoffensive. |
| C3 | **Numérotation des écrans** | **S3 (primaire, manuel +3)** : « screen 0 is held in RAM 7 » | **S1/S2 (primaire)** : « Page 7 — screen 2 », « Page 5 — screen 1 » ; S7 : normal (0) = banque 5 | **S3 est un erratum** de numérotation. La sémantique du bit n'est contestée nulle part : bit 3 = 0 → RAM 5, = 1 → RAM 7. Ne parler qu'en numéros de banque. |
| C4 | **Valeurs absolues de T-states +2A/+3** | **S7** : table à partir de 14365 | **S15** : table à partir de 14361 | **Non résolu** — décalage plat de 4 T-states entre deux secondaires. Aucun primaire ne donne de valeur absolue (S4 ne donne que les rapports). À calibrer empiriquement. |
| C5 | **Noms des ROM +2A/+3** | **S3/S5 (primaire)** : editor / syntax / DOS / 48 BASIC | **S12 (secondaire)** : « 128K ROM-1 » / « 128K ROM-2 » / « +3DOS ROM » / « 48K ROM » | **Pas un vrai conflit** : même numérotation, conventions de nommage différentes. Utiliser les noms primaires. |
| C6 | **Libellés du champ de bits `&1FFD`** | **S3 (primaire)**, ligne d'en-tête : « D0…D1 - ROM/RAM switching, D2 - Affects whether… » | S3 lui-même dans son texte suivi et ses deux tableaux, plus S7/S12/S14 : D0 = mode, D1-D2 = sélecteurs | **La ligne d'en-tête de S3 est mal libellée** ; son texte suivi et ses tableaux font foi et concordent avec les secondaires. |

**Non trouvé, côté ZX** — à écrire tel quel dans tout profil :

- **Masques de décodage partiel de `&7FFD` et `&1FFD` sur +2A/+3 en source
  primaire.** Le manuel utilisateur +3 ne les donne jamais ; S17 est un scan
  image ; S18 ne traite pas la pagination. Les masques cités reposent sur S7,
  S12, S14 et la table de ports de Fuse, qui concordent exactement.
- **Fonction des bits 5, 6, 7 de `&1FFD`**, et s'ils sont même mémorisés.
- **Fonction des bits 6, 7 de `&7FFD` sur +2A/+3.** (Sur 128/+2 il est *prouvé*
  qu'ils ne font rien : seuls D0-D5 atteignent le 74LS174 — S1/S2 ¶ 4.12.11,
  primaire.)
- **Si lire `&7FFD` ou `&1FFD` perturbe la pagination sur +2A/+3.** Aucune source
  ne l'aborde.
- **Si le HAL corrigé des +2 gris tardifs a aussi changé le schéma de
  contention** — S13 dit explicitement « It is not known ».
- **Si les ports `&7FFD` / `&1FFD` sont contended en entrée-sortie sur +2A/+3** —
  S7 dit explicitement « currently unknown ».

### MSX

1. **Relisibilité des ports de mapper** — M5 dit « not reliable, should not be
   done » ; openMSX implémente des registres relisibles avec un
   `MapperReadBackBits` par machine. **Réconcilié par le primaire** : M3 § 5.7,
   p. 30 — les registres *sont* lus, mais « the top bits of the segment numbers
   are generally not recorded ». Pas de contradiction réelle ; la largeur dépend
   de la machine.
2. **Lecture de `&FFFF` : « complement » contre « reversed »** — M1 § 1.6.2 dit
   « complement » ; la traduction de M2 dit « reversed value ». Le `0xFF ^ value`
   d'openMSX tranche : **complément à un**, pas inversion de l'ordre des bits.
3. **Affectation port par port de `&FC`-`&FF`** — la carte officielle (M2
   annexe 6) ne donne que la plage « FCH to FFH : memory mapper ». L'association
   `FC` → page 0 … `FF` → page 3 vient de M5 (secondaire) et d'openMSX
   (`port & 3` → page), qui concordent. **Aucun texte primaire ne l'énonce** : à
   traiter comme très bien attesté, non confirmé sur documentation constructeur.
4. **Les mappers de mega-ROM n'ont aucune spécification primaire** dans les
   sources atteignables. Tout le § B.5 repose sur openMSX plus les mesures sur
   cartouche de M6 — qui concordent sur chaque adresse et chaque taille de
   banque, sans un seul écart. Le scan du MSX Datapack vol. 3 (japonais, sans
   OCR) n'a pas été lu et pourrait contenir une spécification ASCII8/ASCII16,
   puisque c'est ASCII qui a fait ces puces : ni confirmé ni infirmé.
5. **Le wiki msx.org** a renvoyé HTTP 403 : non consulté, donc impossible de dire
   s'il contredit quoi que ce soit.
6. **`MSXPPI::peekA()` renvoyant 0** dans openMSX est une approximation
   documentée par openMSX lui-même (« TODO check this on a real MSX »),
   applicable seulement si le port A est programmé en entrée. Ne contredit pas le
   `A8H R/W` du Technical Data Book.

---

## Verdict sur la spec

### Bloc `TARGET zx128` (§ 13.2)

| Ligne de la spec | Verdict | Valeur correcte / nuance |
|------------------|---------|--------------------------|
| `WINDOW rom [&0000..&3FFF] READONLY` | **confirmé** pour 128 / +2 ; **à nuancer** pour +2A / +3 | Sur 128/+2, `&0000`-`&3FFF` est toujours de la ROM (S1/S2 § MEMORY ORGANISATION, primaire). Sur +2A/+3, le mode spécial (`&1FFD` bit 0 = 1) y met de la **RAM** — donc `READONLY` n'y est vrai qu'en mode normal (S3, primaire). |
| `WINDOW low [&4000..&7FFF] ALWAYS bank5` | **confirmé** | S1/S2 § MEMORY ORGANISATION (primaire) ; S4 pour +2A/+3 en mode normal (primaire). **À nuancer sur +2A/+3** : en mode spécial cette fenêtre porte la RAM 1, 5 ou 7 selon la configuration. |
| `WINDOW low … CONTENDED` | **à nuancer** | La *valeur* est juste par accident sur 128/+2 (la banque 5 y est effectivement contended), mais la *modélisation* est fausse : la contention est un attribut de la **banque**, pas de la fenêtre. Sur 128/+2 (matériel réel) : banques **1, 3, 5, 7**. Sur +2A/+3 : banques **4, 5, 6, 7**. Conséquence directe : `&C000`-`&FFFF` est contended ou non *selon la banque paginée*, et sur +2A/+3 en mode spécial la RAM 4 en `&0000` rend le **bas** de l'espace adressable contended. Un `CONTENDED` porté par la fenêtre ne peut pas exprimer cela. |
| commentaire « `CONTENDED` — ZX : `&4000`-`&7FFF` » (§ 13.1) | **à nuancer** — même motif | Vrai comme raccourci, faux comme modèle. Voir ci-dessus. |
| `WINDOW mid [&8000..&BFFF] ALWAYS bank2` | **confirmé** | S1/S2 § MEMORY ORGANISATION (primaire) ; S4 (primaire). Même nuance mode spécial +2A/+3 : RAM 2 ou RAM 6. |
| `WINDOW high [&C000..&FFFF] HOSTS bank0..bank7` | **confirmé** | S1/S2 ¶ 4.3 (primaire) : « Any RAM page can occupy the space. » |
| `SELECT high, bank<n> = OUT &7FFD, …` | **confirmé** | S1/S2 ¶ 4.3 et ¶ 4.12.11 (primaire). |
| bits 0-2 : banque | **confirmé** | S1/S2 ¶ 4.3 (primaire). |
| bit 3 : écran normal/shadow | **confirmé** | S1/S2 ¶ 4.3 et § 5.3.3 (primaire) : 0 → page 5, 1 → page 7. Précision utile à ajouter : le bit change l'écran **affiché**, pas la cartographie ; la page 7 n'est accessible au Z80 qu'en `&C000`. |
| bit 4 : ROM 128/48K | **confirmé** pour 128 / +2 ; **à nuancer** pour +2A / +3 | Sur 128/+2 : 0 → ROM 0 (128K), 1 → ROM 1 (48K) (S1/S2 ¶ 4.12.2, primaire). Sur +2A/+3 c'est le **bit de poids faible** d'un numéro de ROM sur 2 bits, le bit de poids fort étant le bit 2 de `&1FFD` (S3, primaire). Le libellé « ROM 128/48K » est donc faux sur +2A/+3. |
| bit 5 : verrou (`LOCKS`) | **confirmé** | S1/S2 ¶ 4.12.12 (primaire) : effaçable **uniquement** par le bouton RESET ou coupure d'alimentation. **À compléter** : le verrou porte sur la pagination entière, pas sur un port — sur +2A/+3 il bloque aussi `&1FFD` (Fuse, primaire-pour-le-modèle). |
| commentaire « `LOCKS` — ZX : bit 5 du port `&7FFD` » (§ 13.1) | **confirmé** | Même source. |
| « le port est en **écriture seule**, sans relecture possible » | **confirmé**, mais **incomplet et dangereux tel quel** | L'écriture seule est confirmée (S3 prescrit les variables système `BANKM = &5B5C` et `BANK678 = &5B67`, primaire). Ce que la spec omet : sur 128 et +2 gris précoces, une **lecture n'est pas neutre** — le décodage ne distingue pas lecture et écriture (S1/S2 ¶ 4.12.11, primaire), et un `IN A,(&7FFD)` échantillonne le bus flottant dans le registre, ce qui plante typiquement la machine (S13, secondaire). |
| « une section de code au timing critique n'a rien à y faire [en `&4000`] » | **confirmé** dans sa conclusion | Vrai, mais pour une raison que la spec formule mal : c'est parce que la banque 5 est contended sur les deux familles, pas parce que la fenêtre l'est. Chiffre primaire à citer (S3, +2A/+3) : des NOP en RAM contended donnent 2,66 MHz effectifs contre 3,55 MHz, soit environ 25 % de perte. |
| **absent de la spec** — port `&1FFD` et le mode spécial des +2A/+3 | **non couvert** | Quatre configurations all-RAM : `(0,1,2,3)`, `(4,5,6,7)`, `(4,5,6,3)`, `(4,7,6,3)` selon les bits 2 et 1 de `&1FFD`, bit 0 = 1 (S3, primaire ; concordance Fuse et deux secondaires). En config 0 **rien n'est contended**. Un profil `zx128` ne peut pas décrire un +3 : ce sont deux cibles distinctes. |
| **absent de la spec** — décodage partiel du port | **non couvert, et important** | Sur 128/+2, le registre répond à **toute** adresse d'entrée-sortie avec A15 = 0 et A1 = 0 (S1/S2 ¶ 4.12.11, primaire), soit 16 384 alias — dont `&0FFD`, `&1FFD`, `&2FFD`, `&3FFD`. Sur +2A/+3 le décodage est différent (A14 = 1 en plus pour `&7FFD`), mais **aucune source primaire ne l'établit** : S7, S12, S14 et Fuse concordent, ce n'est pas confirmé sur documentation constructeur. |

### Bloc `TARGET msx-konami` (§ 13.2)

| Ligne de la spec | Verdict | Valeur correcte / nuance |
|------------------|---------|--------------------------|
| quatre `WINDOW page0..page3` de 16 K sur `&0000`-`&FFFF` | **confirmé** pour le découpage en slots | M2 § 7.1.1 et M1 § 1.7.1, § 2.2.3 (primaire). Point à ajouter, structurant : une page ne peut pas changer de numéro — « page n of each slot is also page n to the CPU » (M2, primaire). Fenêtre et page sont la même chose sur MSX. |
| « chacune choisit indépendamment quel *slot* est visible » | **confirmé** | M1 § 1.7.11, p. 43 (primaire) : deux bits par page dans le port A du PPI. |
| `SELECT page<p>, slot<s> = OUT &A8, …` | **confirmé** | `&A8` est bien le port A du PPI i8255, et c'est bien le registre de sélection de slot primaire : M1 § 1.7.6 p. 41, § 1.7.1 p. 38, § 1.6.2 p. 36, et M2 annexe 6 (quatre attestations primaires). |
| commentaire « 2 bits par page » | **confirmé**, à compléter par l'ordre | M1 § 1.7.11 p. 43 (primaire) : bits 0-1 → page 0, 2-3 → page 1, 4-5 → page 2, 6-7 → page 3. Page 0 dans les bits de poids faible. |
| **implicite dans la spec** — que `OUT &A8` suffise à sélectionner un slot | **à nuancer** | Insuffisant pour un slot **étendu** : il faut en plus écrire dans le registre mappé en mémoire à `&FFFF`, deux bits par page dans le même ordre (M2 § 7.1.2 fig. 5.39 ; M1 § 1.6.2 p. 36, primaire). Et ce registre est celui du slot primaire **sélectionné en page 3** : commuter un sous-slot en page 1 exige une gymnastique documentée (M5, secondaire). Un `SELECT` MSX complet est donc une **suite** d'écritures de natures différentes — ce que le vocabulaire du § 13.1 permet, mais que le bloc d'exemple ne montre pas. |
| **absent de la spec** — relecture de `&FFFF` | **non couvert** | Le registre `&FFFF` se relit **complémenté** (`0xFF ^ valeur`), délibérément, pour le distinguer de la RAM ordinaire : M1 § 1.6.2 p. 36 (primaire, littéral) ; circuit de référence M1 § 1.5.5 p. 35 ; `0xFF ^ subSlotRegister[...]` dans openMSX. |
| `SELECT page2, rombank<n> = POKE &8000, <n>` — le **mécanisme** : écriture mémoire | **confirmé** | Aucun des quatre mappers étudiés (Konami4, Konami5/SCC, ASCII8, ASCII16) n'utilise de port d'entrée-sortie : tous implémentent `writeMem` et aucun `writeIO` (openMSX, primaire-pour-le-modèle) ; M6 (secondaire, mesuré sur cartouche) le dit aussi. **Le `POKE` du § 13.1 est donc justifié.** À signaler dans le profil : ces mappers n'ont **aucune spécification primaire** — leurs valeurs viennent d'openMSX et de reverse engineering concordants. |
| `SELECT page2, rombank<n>` — la **granularité** : une banque par page de 16 K | **réfuté pour Konami** | La banque Konami (avec ou sans SCC) fait **8 K**, pas 16 K. Il y a **quatre** fenêtres de 8 K sur `&4000`-`&BFFF`, pas deux de 16 K. Seul **ASCII16** a des banques de 16 K, et sur deux fenêtres seulement (`&4000`-`&7FFF`, `&8000`-`&BFFF`). Sources : commentaires d'en-tête et code de `RomKonami.cc`, `RomKonamiSCC.cc`, `RomAscii16kB.cc` (openMSX) ; tables de M6. **Conséquence de modèle** : un profil MSX à cartouche a besoin de fenêtres de 8 K, qui ne coïncident donc pas avec les pages de slot de 16 K. Deux découpages différents se superposent. |
| `POKE &8000, <n>` — l'**adresse** | **à nuancer selon le mapper** | Pour **Konami sans SCC** : `&8000` commute bien la fenêtre `&8000`-`&9FFF`, et c'est même l'adresse que les jeux utilisent (openMSX `RomKonami.cc` ; M6). Pour **Konami avec SCC**, `&8000` ne commute **rien** : le registre de cette fenêtre est en `&9000`-`&97FF` (openMSX `RomKonamiSCC.cc`). Or le § 13.2 nomme le profil `msx-konami` sans dire lequel des deux. **Il faut choisir**, ce sont deux cibles distinctes. |
| **absent de la spec** — la fenêtre `&4000` fixe de Konami4 | **non couvert, et confirmé comme réel** | Sur Konami **sans** SCC, `&4000`-`&5FFF` **n'a pas de registre** : elle est figée au segment 0. openMSX : « page at 4000 is fixed » ; M6 le confirme par le comportement de « The Game Master 2 ». Sur Konami **avec** SCC, cette fenêtre *est* commutable (`&5000`-`&57FF`). C'est un `ALWAYS bank0` que seul un profil peut porter. |
| **absent de la spec** — mappers ne couvrant que `&4000`-`&BFFF` | **non couvert** | Aucun de ces quatre mappers ne pagine `&0000`-`&3FFF` ni `&C000`-`&FFFF` (les quatre `WINDOW page0..page3` du bloc suggèrent le contraire). Les pages 0 et 3 sont non mappées après reset chez ASCII8 et ASCII16 (openMSX). Le miroitage diffère d'un mapper à l'autre, et openMSX signale explicitement que celui de Konami4 diffère de celui de Konami5. |
| « écrire une donnée dans la plage d'un mapper commute par accident » | **confirmé** | openMSX, `RomKonami.cc` : les adresses canoniques « are used by the games, but **any other address in a page switches that page as well** ». M6 : « if it is a memory area, any address in the area will do the same thing », et le cas de « Hai no Majutsushi », qui commute via `A000h` **et** `B000h`, prouve que le décodage partiel est la réalité du matériel. |
| l'**étendue** du piège | **à préciser, et elle varie beaucoup** | **Konami4** : toute écriture entre `&6000` et `&BFFF` commute — 24 Ko sans aucune zone de données sûre. **Konami5/SCC** : seulement les 2 Ko `x000`-`x7FF` de chaque bloc de 8 K ; l'autre moitié est sûre (sauf `&9800`-`&9FFF` quand le SCC est actif). **ASCII8** : seulement `&6000`-`&7FFF`. **ASCII16** : seulement `&6000`-`&67FF` et `&7000`-`&77FF`. Un attribut unique ne suffit pas : le profil doit porter une **plage de commutation** par registre. Sources : gardes de `writeMem` et surcharges `getWriteCacheLine()` d'openMSX ; tables de M6. |
| **absent de la spec** — le recouvrement registre / SCC | **non couvert** | Sur Konami5, `&9000`-`&97FF` est **à la fois** le registre de banque de la fenêtre `&8000`-`&9FFF` et le registre d'activation du SCC (valeur dont les bits 0-5 sont à 1) : une seule écriture fait les deux (openMSX ; M6). |
| **absent de la spec** — memory mapper RAM | **non couvert** | Celui-là, contrairement aux mega-ROM, commute bien par **port d'entrée-sortie** : `&FC`-`&FF`, un par page, granularité 16 K. La plage est primaire (M2 annexe 6) ; l'association port ↔ page ne vient que de M5 et openMSX, concordants, **non confirmée sur primaire**. Relecture non fiable : seuls les bits implémentés remontent (M3 § 5.7 p. 30, primaire). C'est ce contraste `OUT` / `POKE` **au sein d'une même machine** qui justifie le mieux le § 13.1. |
