# Gate Array, PAL et `RMR` sur Amstrad CPC — vérification sur sources primaires

Objet : établir, sur sources faisant autorité, le décodage du registre de mode et
de ROM du Gate Array (`RMR`), la sélection de la ROM haute, la configuration de
la RAM étendue par le PAL, et le registre `RMR2` du CPC Plus. Ce document
répond aux questions A à D posées pour vérifier le §7 de
`docs/spec-chaine-outils.md`, dont le §12.3 postule que la **polarité des bits de
ROM** fait l'objet de documentations contradictoires.

Date de la recherche : 2026-09-07. Toutes les URL ont été consultées à cette
date. Certaines ont dû être lues via `web.archive.org` (indiqué le cas échéant).

---

## 0. Inventaire des sources, par niveau d'autorité

### Niveau 1 — Documentation Amstrad officielle

| Réf | Source | Accès |
|-----|--------|-------|
| **[S968]** | Amstrad / Locomotive Software, *SOFT968 — The Amstrad CPC464/664/6128 Firmware Manual*. Sections citées : §2.2 « ROM Selection », §2.5 « Bank Switching », §10 « Expansion ROMs… », Appendice « E. Video Gate Array » (sous-sections *MODE AND ROM ENABLE REGISTER*, *BANK SWITCHING REGISTER (CPC6128 only)*), Appendice « K. I/O ports ». | Texte intégral OCR : <https://archive.org/stream/SOFT968TheAmstrad6128FirmwareManual/SOFT968%20-%20The%20Amstrad%206128%20Firmware%20manual_djvu.txt> — fiche : <https://archive.org/details/SOFT968TheAmstrad6128FirmwareManual> ; fac-similés PDF par appendice : <https://cpctech.cpcwiki.de/docs.html> (`docs/manual/s968ap12.pdf` etc.) |

C'est la seule source *émise par le constructeur* utilisée ici. Elle tranche
tous les points qu'elle couvre.

### Niveau 2 — Documentation technique matérielle de référence

| Réf | Source | Accès |
|-----|--------|-------|
| **[GRIM-GA]** | Grimware (Grim / Ludovic Deplanque), *documentations:devices:gatearray*. Sections : « I/O Decoding », « Registers », « RMR », « Upper ROM », « Lower ROM », « RMR2 », « MMR », « Regular RMR Register ». Documentation établie par mesure et rétro-ingénierie sur 40010 / 40007 / ASIC. | <https://www.grimware.org/doku.php/documentations/devices/gatearray> (site en panne HTTP 522 le 2026-09-07 ; lu via <http://web.archive.org/web/2020id_/http://www.grimware.org/doku.php/documentations/devices/gatearray>) |
| **[GRIM-IO]** | Grimware, *documentations:devices:io.devices*, section « I/O Ports map » (masques d'adresse bit à bit). | <http://web.archive.org/web/2019/http://www.grimware.org/doku.php/documentations/devices/io.devices> |
| **[CT-GA]** | Kevin Thacker (auteur de l'émulateur **Arnold**), *Amstrad CPC Gate-Array*. | <https://cpctech.cpcwiki.de/docs/garray.html> |
| **[CT-MEM]** | K. Thacker, *mem — Memory selection hardware* : tables de vérité du PAL du CPC6128, reconstituées à partir du **service manual** du CPC6128. | <https://cpctech.cpcwiki.de/docs/mem.html> |
| **[CT-EXPROM]** | K. Thacker, *Expansion ROM Selection*. | <https://cpctech.cpcwiki.de/docs/exprom.html> |
| **[CT-IOPORD]** | K. Thacker, *I/O port decoding* : tableau des bits d'adresse réellement décodés par chaque périphérique. | <https://cpctech.cpcwiki.de/docs/iopord.html> |
| **[CT-GAINT]** | K. Thacker, *Interrupt Generation Facility of the Amstrad Gate Array*. | <https://cpctech.cpcwiki.de/docs/gaint.html> |
| **[LOGON35]** | Logon System — Fred Crazy, « Le Gate Array », *Amstrad Cent Pour Cent* n°35. Documentation d'époque du groupe de Longshot. | <https://cpcrulez.fr/coding_logon35-le_gate_array.htm> |

### Niveau 3 — Ouvrage imprimé d'époque (tiers, non constructeur)

| Réf | Source | Accès |
|-----|--------|-------|
| **[WMG]** | *Amstrad Whole Memory Guide*, chapitre « General system arrangement ». | <https://www.cpcwiki.eu/index.php/Amstrad_Whole_Memory_Guide_-_General_system_arrangement> (403 en accès direct ; lu via <http://web.archive.org/web/2023id_/https://www.cpcwiki.eu/index.php/Amstrad_Whole_Memory_Guide_-_General_system_arrangement>) |

### Niveau 4 — Wiki (secondaire, compilateur des précédents)

| Réf | Source | Accès |
|-----|--------|-------|
| **[CW-GA]** | CPCWiki, *Gate Array* (oldid 112797, 12 octobre 2023). | <https://www.cpcwiki.eu/index.php/Gate_Array> (lu via web.archive.org) |
| **[CW-UROM]** | CPCWiki, *Upper ROM Bank Number* (oldid 118850). | <https://www.cpcwiki.eu/index.php/Upper_ROM_Bank_Number> (lu via web.archive.org) |
| **[CW-MEMEXP]** | CPCWiki, *Standard Memory Expansions*. | <https://www.cpcwiki.eu/index.php/Standard_Memory_Expansions> (lu via web.archive.org) |

**Sources non obtenues.** Le *Compendium* de Longshot / Logon System
(<https://shaker.logonsystem.eu/>), référence sur CRTC et Gate Array, n'a pas pu
être consulté dans le temps de cette recherche : ses affirmations ne figurent
donc pas ici. La documentation ASIC officielle du CPC Plus (citée par [GRIM-GA]
et [CT-GA] sous le nom « official ASIC documentation ») n'a pas été lue
directement non plus ; les points D reposent sur [GRIM-GA], [CT-GA] et
[CW-UROM]. Le service manual du CPC6128 n'a pas été lu en original : il est
cité *par* [CT-MEM], qui s'en réclame.

---

## A. Gate Array — registre de sélection de mode et de ROM (`RMR`)

Note de nomenclature : Amstrad ne nomme pas ce registre `RMR`. [S968] l'appelle
« **MODE AND ROM ENABLE REGISTER** ». Le sigle `RMR` provient de [GRIM-GA]. Les
deux désignent le même registre.

### A.1 — Sélection du registre et plage de ports (question 1)

**Sélection par les bits de poids fort de la donnée écrite.**

[S968], appendice « E. Video Gate Array », mot pour mot :

> « One I/O channel is used for all commands, the top two bits of data
> specifying the command type as follows: »
>
> | Bit 7 | Bit 6 | Use |
> |---|---|---|
> | 0 | 0 | Load palette pointer register. |
> | 0 | 1 | Load palette memory. |
> | 1 | 0 | Load mode and ROM enable register. |
> | 1 | 1 | Bank Switching Register on CPC6128. |

Donc `RMR` est sélectionné par **bit7 = 1, bit6 = 0**. [CT-GA], [CW-GA],
[GRIM-GA] et [LOGON35] concordent. [LOGON35] : « si le bit 7 est à 1 et le 6 à
0 […] nous sommes tout simplement dans le cas de la commutation des Roms et du
contrôle vidéo ».

Le bit 5 est **réservé** : [S968] écrit « Bit 5: ** Reserved ** (send 0) ».
[GRIM-GA] précise ce que cela vaut en pratique :

> « On a real Gate Array (CPC 464,664 and 6128), bit5 of the command have
> absolutly no effect and can be set to 0 or 1. However, on a Plus machine with
> it's RMR2 register unlocked, this bit is used to select the RMR2 register. »
> — [GRIM-GA], « Bit5 and compatibility between CPC and Plus »

**Plage de ports et bits d'adresse réellement décodés.**

[S968], appendice « K. I/O ports » : `#7Fxx` → « Video Gate Array » en sortie,
« **Do not use** » en entrée. [S968] donne l'adresse recommandée, pas le
décodage.

Le décodage réel est donné par [CT-GA], mot pour mot :

> « The gate array is selected when bit 15 of the I/O port address is set to "0"
> and bit 14 of the I/O port address is set to "1". The values of the other bits
> are ignored. However, to avoid conflict with other devices in the system,
> these bits should be set to "1".
> The recommended I/O port address is &7Fxx. »

[CW-GA] reprend cette phrase à l'identique. [GRIM-IO], tableau « I/O Ports map »,
donne le masque : `GateArray | W | &7F00 | 01 xxxxxx xxxxxxxx` — soit A15 = 0,
A14 = 1, tout le reste indifférent. [GRIM-GA] le redit en clair :

> « The PAL only test bit15 of the I/O address while the Gate Array test bit15
> and bit14. »

Le Gate Array répond aussi en lecture, ce qui est un piège :

> « the PAL only respond to I/O Write requests while the Gate Array will respond
> no matter what the Read/Write I/O signal is. If you execute an I/O read
> operation on the Gate Array I/O address, the Gate Array will read an
> unpredictable value from the databus which will be in high-impedance state.
> If the value is a valid Gate Array command, it will be executed »
> — [GRIM-GA], « I/O Decoding »

**Divergence relevée.** [WMG] écrit : « If address bit A15 is low, the Video
Gate Array is selected. This port is for output only. The address must be 7FXX. »
Cette formulation **omet la condition A14 = 1** et est donc trop large : le
masque « A15 = 0 seul » est celui du PAL, pas celui du Gate Array. Autorité :
[CT-GA] / [GRIM-IO] / [CT-IOPORD] l'emportent, parce qu'ils distinguent
explicitement les deux décodages et que [CW-MEMEXP] confirme la distinction
(« PAL16L8 decodes A15=0 only / Gate-Array decodes A15=0 and A14=1 »). [WMG]
reste juste sur la conclusion pratique (« the address must be 7FXX »).

### A.2 — Disposition des bits, et polarité des bits de ROM (question 2) — **point central**

**Source de niveau 1, mot pour mot** — [S968], appendice « E. Video Gate
Array », sous-section *MODE AND ROM ENABLE REGISTER* :

> « This write-only register controls the VDU mode and ROM enabling as follows:
>
> Bit 7: 1
> Bit 6: 0
> Bit 5: ** Reserved ** (send 0)
> Bit 4: Clear raster 52 divider.
> **Bit 3: Upper half ROM disable.**
> **Bit 2: Lower half ROM disable.**
> Bit 1: VDU Mode control MC1.
> Bit 0: VDU Mode control MC0. »

et, plus bas, la valeur au reset :

> « On power-up and other system resets, the mode and ROM enable register is set
> to zero, enabling both halves of the ROM. »

Cette dernière phrase est décisive : registre à **zéro** ⇒ **les deux ROM sont
actives**. Donc **0 active, 1 inhibe**.

**Réponse à la question centrale : la valeur 1 INHIBE la ROM et laisse voir la
RAM. La valeur 0 ACTIVE la ROM.** Pour les deux bits, ROM basse (bit 2) comme
ROM haute (bit 3).

Les autres sources, mot pour mot :

- [GRIM-GA], section « RMR » :
  > « **UR : Enable (0) or Disable (1) the upper ROM paging (&C000 to &FFFF).**
  > You can select which upper ROM with the I/O address &DF00.
  > **LR : Enable (0) or Disable (1) the lower ROM paging.** »

  et, section « Lower ROM » : « The RMR Bit2 (LR) control the lower ROM paging
  **ON(0)/OFF(1)**. » Son exemple de code enfonce le clou : « `ld bc,&7F00 +
  %10000110` ; change the RMR register to **enable** the Upper ROM paging
  (**bit3=0**) » puis « ; And now we **disable** the Upper ROM paging /
  `ld bc,&7F00 + %10001110` ; **bit3=1** ».

- [CT-GA], tableau « Summary » du registre :
  > | Bit | Value | Function |
  > |---|---|---|
  > | 3 | 1 | Upper rom area **disable** |
  > | 3 | 0 | Upper rom area **enable** |
  > | 2 | 1 | Lower rom area **disable** |
  > | 2 | 0 | Lower rom area **enable** |

  et son exemple : « Mode 2, upper and lower rom **disabled**. `LD
  A,%10000000+%00001110` » — bits 3 et 2 à 1.

- [CW-GA], même tableau : « 3 | x | **1=Upper ROM area disable, 0=Upper ROM area
  enable** » et « 2 | x | **1=Lower ROM area disable, 0=Lower ROM area
  enable** ».

- [LOGON35], en français, mot pour mot :
  > « **Si le bit 3 est à 1, La Rom supérieure (de #C000 à #FFFF) sera
  > déconnectée; dans le cas contraire, elle sera connectée. Si le bit 2 est à
  > 1, la Rom inférieure (de #0000 à #3FFF) sera déconnectée, dans le cas
  > contraire connectée.** »

- [WMG] :
  > « The instructions for switching between ROM and RAM are given by outputs to
  > bits 2 and 3 of port 7FXX. **A 1 disables, a 0 enables**, while bit 2
  > applies to the lower ROM and bit 3 to the upper ROM. »

- [CT-EXPROM], en commentaire de code : « `ld a,%10000100` ; **enable upper rom,
  disable lower rom, mode 0** » — bit 3 = 0, bit 2 = 1.

**Sept sources indépendantes, de quatre niveaux d'autorité, disent la même
chose. Aucune source consultée n'affirme le contraire.** Voir §E.2 pour la
conclusion sur l'existence supposée d'une contradiction.

**Sémantique exacte de « activée ».** L'inhibition ne concerne que la lecture ;
l'écriture va toujours en RAM. [S968] §2.2 :

> « When the upper ROM is enabled data read from addresses between #C000 and
> #FFFF is fetched from the ROM. Similarly, when the lower ROM is enabled data
> read from addresses between #0000 and #3FFF is fetched from the ROM. When the
> ROMs are disabled data is fetched from RAM. **Note that the ROM state does not
> affect writing which always changes the contents of RAM.** »

Idem [GRIM-GA] : « all CPU write operations between #C000 to #FFFF will affect
the "underlying" RAM (depending on the MMR configuration) ».

**Disposition retenue** (identique sur CPC 464, 664, 6128 ; sur Plus ASIC
déverrouillé, bit 5 = 1 sélectionne `RMR2` au lieu de `RMR`) :

```
bit  7 6 5 4 3 2 1 0
     1 0 r I U L M M
         │ │ │ │ └─┴── MM : mode écran (00..11)
         │ │ │ └────── L  : ROM basse — 0 = active, 1 = inhibée (RAM visible)
         │ │ └──────── U  : ROM haute — 0 = active, 1 = inhibée (RAM visible)
         │ └────────── I  : 1 = agit sur le compteur d'interruption (cf. A.3)
         └──────────── r  : réservé sur CPC (envoyer 0) ; sur Plus déverrouillé,
                            1 = sélectionne RMR2
reset : 00000000 → les deux ROM actives, mode 0
```

### A.3 — Bit 4 : compteur d'interruption (question 3)

Le bit est **à 0 au repos** : [S968] indique que le registre entier vaut zéro au
reset ; [S968] écrit par ailleurs « Bit 5: ** Reserved ** (send 0) » mais rien
d'équivalent pour le bit 4, dont la mise à 1 est une action ponctuelle (« Writing
a 1 to bit 4 clears… »). Toutes les sources s'accordent : on n'écrit 1 que
lorsqu'on veut l'effet, et 0 sinon.

**Ce que fait le bit — contradiction réelle entre sources.**

- [S968], mot pour mot : « Bit 4: **Clear raster 52 divider.** […] Writing a 1
  to bit 4 **clears the top bit** of the divide by 52 counter used for
  generating periodic interrupts. »
- [GRIM-GA] : « **I : if set (1), this will reset the interrupt counter.** » et,
  section Interrupts : « the internal interrupt counter can be cleared anytime by
  software using the Gate Array RMR register. »
- [CT-GAINT] : « The GA has a software controlled interrupt delay feature. **The
  GA scan line counter will be cleared immediately** upon enabling this option
  (bit 4 of ROM/mode control). It only applies once and has to be reissued if
  more than one interrupt needs to be delayed. » Et, distinctement : « Once the
  Z80 acknowledges the interrupt, the GA clears **bit 5** of the scan line
  counter. »
- [LOGON35] : « le bit 4, s'il est à 1, **remettra à 0 le diviseur** qui génère
  les interruptions. L'interruption débute alors 52 lignes plus loin ! »
- [CT-GA] / [CW-GA], plus prudents : « Bit 4 controls the interrupt generation.
  It can be used to delay interrupts. »

**Arbitrage.** [S968] dit « clears the top bit », les trois autres disent
« remet le compteur entier à 0 ». Les deux ne peuvent pas être vraies :
effacer seulement le bit de poids fort d'un compteur modulo 52 ne le remet pas à
zéro. [CT-GAINT] attribue précisément l'effacement du seul bit 5 à un *autre*
événement — l'acquittement d'interruption par le Z80 — ce qui suggère que [S968]
confond les deux mécanismes, ou décrit une implémentation antérieure.

Sur ce point précis, **la source de niveau 1 est la moins fiable**, contre
l'ordre habituel : [CT-GAINT], [GRIM-GA] et [LOGON35] sont issus de
l'observation du silicium et de l'écriture d'émulateurs, et concordent entre
eux ; l'effet « l'interruption arrive 52 lignes plus tard » rapporté par
[LOGON35] n'est explicable que par une remise à zéro complète. **Retenu :
1 remet le compteur de lignes (diviseur par 52) à 0, effet ponctuel, à réémettre
pour chaque interruption à décaler.** Signalé comme contradiction non levée par
mesure dans le cadre de cette recherche.

**Nommage.** Aucune source ne parle de « compteur vsync » pour ce bit. Le
compteur en question est incrémenté sur **HSYNC** : [CT-GAINT] « The GA has a
counter that increments on every falling edge of the CRTC generated HSYNC
signal. Once this counter reaches 52, the GA raises the INT signal and resets the
counter to 0. » Le VSYNC intervient ailleurs (il déclenche une action retardée
de 2 HSYNC, puis une comparaison à 32). Appeler le bit 4 « remise à 0 du compteur
vsync » est donc inexact.

### A.4 — Bits 1-0 : mode écran (question 4)

[S968], mot pour mot :

> | MC1 | MC0 | Mode |
> |---|---|---|
> | 0 | 0 | Mode 0, 160 x 200 pixels in 16 colours |
> | 0 | 1 | Mode 1, 320 x 200 pixels in 4 colours. |
> | 1 | 0 | Mode 2, 640 x 200 pixels in 2 colours. |
> | 1 | 1 | **\*\*Do not use\*\*** |

> « The gate array hardware synchronises mode changing to the next horizontal
> flyback in order to aid software that requires different parts of the screen to
> be handled in different modes. »

Le mode 3 **existe** matériellement, ce qu'Amstrad ne documente pas :

- [GRIM-GA] : « `%11 (3)` […] 2bits/pixels (4 colors), 2 pixels/byte
  (160×200) », et « Note that the Gate Array needs an HSync of at least 2μs to
  update it's internal byte⇒pixels decoder with a new video mode. »
- [CT-GA] : « Mode 3, 160x200 resolution, 4 colours (note 1) […] This mode is
  not official. »
- [CW-GA] ajoute une différence entre machines : « Mode 3 is **not supported by
  the KC Compact** (which outputs black in Mode 3). » (le KC compact n'est pas un
  CPC mais un clone est-allemand ; mentionné pour complétude).
- [GRIM-GA] signale une différence de silicium, non de mode : « On a real Gate
  Array 40010, when the video mode is set to 2 (640x200x2c), the display will
  start one pixel (mode 2) earlier than in the others video modes (0,1 and 3) […]
  The video RAM rasterization timings on a 40007 Gate Array and ASIC are not
  affected by the video mode. »

Aucune divergence 464 / 664 / 6128 sur les bits de mode.

---

## B. Sélection du numéro de ROM haute (question 5)

**Port.** [S968], appendice « K. I/O ports » : `#DFxx` → « Expansion ROM
select » en écriture, « **Not used** » en lecture. [S968] §10 : « To select a
given ROM the Kernel sets its ROM address by **writing to I/O address #DF00**. »

**Décodage réel.** [CT-EXPROM], mot pour mot :

> « The rom select I/O address is **not fully decoded**, and at a minimum **bit
> 13 of the I/O address must be set to "0"** for roms to be selected. To avoid
> conflict with other devices, the remaining bits in the I/O address should be
> set to "1". The recommended I/O address is &DFxx.
> The rom select I/O address is write only. »

[GRIM-IO], masque : `Upper ROM | W | &DF00 | xx 0 xxxxx xxxxxxxx` — A13 = 0
seul. [WMG] : « If address bit A13 is low, ROM select data is being output. The
address must be DFXX. » Trois sources concordantes, aucune divergence.

**Plage de numéros.** [S968] §2.2 : « ROM select addresses are in the range
**0...251**, providing for up to 252 expansion ROMs. » C'est la limite
*firmware*. Le matériel accepte les 256 valeurs : [GRIM-GA] « On the CPC, it's
simple, you can select any logical ROM id from 0 to 255 and that's it » ;
[CW-UROM] « Writing to Port DFxxh selects the Upper ROM Bank Number (in range of
**00h..FFh**) ». Pas de contradiction : deux périmètres différents, à noter tels
quels.

**Comportement quand aucune ROM ne porte le numéro désigné.** [S968] §10, mot
pour mot :

> « If a ROM is fitted at the address selected, then all further read accesses to
> the top 16K of memory will return data from the expansion ROM. **If no ROM is
> fitted at the currently selected ROM address the contents of the on-board ROM
> are returned.** »

[CT-EXPROM] dit la même chose sous une autre forme : « **BASIC will be selected
if an attempt is made to select a rom which is not connected.** » — sur CPC, la
ROM haute embarquée *est* le BASIC, les deux formulations coïncident.

[CW-UROM] explique le mécanisme, et pourquoi c'est ce comportement :

> « The ROM Bank Number **is not stored anywhere inside of the CPC**. Instead,
> peripherals must watch the bus for writes to Port DFxxh, check if the Bank
> Number matches the Number where they want to map their ROM to, and memorize
> the result by setting/clearing a flipflop accordingly (eg. a 74LS74). If the
> flipflop indicates a match, and the CPC outputs A15=HIGH (upper memory half),
> then the peripheral should set /OE=LOW (on its own ROM chip), and output the
> opposite level, ROMDIS=HIGH to the CPC (disable the CPC's BASIC ROM). […] **By
> default, if there are no peripherals issuing ROMDIS=HIGH, then BASIC is mapped
> to all ROM banks in range of 00h..FFh.** »

**Le port &DF00 ne rend visible aucune ROM à lui seul** : il faut de plus
activer la ROM haute dans `RMR`. [CT-EXPROM] : « This process will select a rom,
but will not allow access to the rom data. To access the rom data: Select the rom
using the method above, **Enable the upper rom region using the Gate Array** ».

**Différences entre machines.** ROM 0 = BASIC sur toutes. ROM 7 = AMSDOS sur
664 et 6128, et sur 464 **seulement avec une interface DDI-1** ([GRIM-GA] :
« 7 - AmsDOS (664, 6128 or 464 with DDI) » ; [CW-UROM] : « ROM 7: AMSDOS (except
on CPC464) », et « 00h BASIC (or AMSDOS, depending on LK1 on the DDI-1 board) »).
Sur **Plus**, le port change de sémantique — voir §D.

---

## C. Configuration de la RAM (PAL)

### C.1 — Plage de ports, sélection du registre, disposition (question 6)

**Même port que le Gate Array**, mais autre puce. [S968], appendice E, désigne
la fonction bits 7-6 = `11` comme « Bank Switching Register on CPC6128 ».
[S968] §2.5 l'attribue à « **The ULA** in the CPC6128 » ; [CT-GA], [CW-GA],
[GRIM-GA] et [CW-MEMEXP] disent **PAL** (un PAL16L8). Sur ce point de nommage,
les sources de niveau 2 l'emportent : [CT-MEM] déclare travailler sur les
signaux du service manual du CPC6128, et [CW-MEMEXP] nomme la référence exacte
du composant (« This is handled by a PAL16L8 but to the programmer it appears to
be the fourth register in the Gate Array »). Sans conséquence pour un
assembleur.

**Décodage.** [GRIM-IO] : `PAL | W | &7F00 | 0 xxxxxxx xxxxxxxx` — **A15 = 0
seul**. [CT-IOPORD] : « RAM Configuration | Write Only | b15 = 0 » (tous les
autres bits indifférents). [CW-MEMEXP] : « PAL16L8 decodes **A15=0 only** /
Gate-Array decodes A15=0 and A14=1 ». [GRIM-GA] : « The PAL only test bit15 of
the I/O address while the Gate Array test bit15 and bit14. Moreover, the PAL
only respond to I/O Write requests ».

**Contradiction relevée.** [CT-MEM] écrit, pour le même composant : « the memory
select hardware responds to I/O writes only, and the following conditions must be
true: **A15="0", A14="1"**, D7="1", D6="1" ». C'est incompatible avec
[GRIM-IO], [CT-IOPORD] et [CW-MEMEXP], et **[CT-MEM] contredit [CT-IOPORD], du
même auteur**. Autorité : le trio A15-seul l'emporte, parce que (a) [CT-MEM] se
présente lui-même comme une reconstitution — « This document presents **a valid
logic schematic which would perform the same action** as the original chip » —
et précise que « the truth table and logic operation of the signals **are not
known** » ; (b) A14 est de toute façon une entrée du PAL, utilisée pour calculer
A14OUT, ce qui explique facilement qu'elle apparaisse dans une table de vérité
sans faire partie du décodage de port. **Conséquence pratique : nulle si l'on
écrit sur &7F00, qui satisfait les deux lectures.** À ne pas confondre avec le
Gate Array si l'on s'écarte de &7Fxx.

**Sélection et disposition des bits.** [S968], appendice E, sous-section
*BANK SWITCHING REGISTER (CPC6128 only)*, mot pour mot :

> « This write-only register controls the layout of the bank switchable RAM as
> follows:
> Bit 7: 1
> Bit 6: 1
> Bit 5: ** Reserved ** (send 0)
> **Bit 4: 0**
> **Bit 3: 0**
> Bit 2: x
> Bit 1: x
> Bit 0: x
> The xxx appearing on Bits 0, 1, and 2 is the code for the selected bank
> layout »

Amstrad ne prévoit donc **aucun bit de page** sur un CPC6128 : bits 3 et 4 à 0,
bit 5 réservé. Le champ `ppp` est une extension *de fait* — voir C.3.

Disposition de fait, [CW-MEMEXP], mot pour mot :

> « Port 7Fxxh:
> **7-6 Must be both set to 1**
> **5-3 Bank number in 64K units** (for expansions bigger than 64K) (max = 512K)
> **2.0 RAM configuration as on CPC6128** taken from the selected bank »

[CW-GA] donne la même chose : « 5 | b | 64K bank number (0..7); always 0 on an
unexpanded CPC6128, 0-7 on Standard Memory Expansions » (bits 5, 4, 3), et
« 2 | x | RAM Config (0..7) » (bits 2, 1, 0). [GRIM-GA] idem, en nommant les
champs : `MMR | 1 1 | 64K page = bits 5,4,3 | S = bit 2 | MM = bits 1,0`.

Soit `11pppccc`, avec `ppp` = bits 5-3 (page de 64 K) et `ccc` = bits 2-0
(configuration). **La disposition du §7 est confirmée.**

Différence entre machines : le registre **n'existe pas** sur CPC 464 et 664
sans extension. [CW-GA] : « This register exists only in CPCs with 128K RAM (like
the CPC 6128, or CPCs with Standard Memory Expansions). » [CT-GA] : « In the
CPC464, CPC664 and KC compact, this function is performed in a memory-expansion
(e.g. Dk'Tronics 64K Ram Expansion), **if this expansion is not present then the
function is not available.** In the CPC6128, this function is performed by a PAL
located on the main PCB, or a memory-expansion. In the 464+ and 6128+ this
function is performed by the ASIC or a memory expansion. »

### C.2 — Table des 8 configurations (question 7)

**Source de niveau 1**, [S968] §2.5 « Bank Switching », mot pour mot :

> « The 128K of bank switched RAM is split into 8 16K blocks, numbered 0..7.
> There are 8 memory organizations, also numbered 0...7, each of which switches a
> different set of four blocks into the memory map at #0000..#3FFF, #4000..#7FFF,
> #8000..#BFFF and #C000..#FFFF. […] The blocks available in each organization
> are as follows: »

| Organization | #0000 | #4000 | #8000 | #C000 |
|---|---|---|---|---|
| 0 | 0 | 1 | 2 | 3 |
| 1 | 0 | 1 | 2 | 7 |
| 2 | 4 | 5 | 6 | 7 |
| 3 | 0 | 3 | 2 | 7 |
| 4 | 0 | 4 | 2 | 3 |
| 5 | 0 | 5 | 2 | 3 |
| 6 | 0 | 6 | 2 | 3 |
| 7 | 0 | 7 | 2 | 3 |

(Blocs 0-3 = 64 K de base ; blocs 4-7 = les 16 K × 4 de la page étendue.)

[S968], appendice E, redonne la même table transposée et ajoute :

> « **Selecting Code 0 (the number sent to the Bank Switch Register would be
> #C0) will switch to the normal default bank layout, i.e. Banks 0..3.** The
> other 64k can be accessed by selecting codes 4..7. These switch one of the
> other 16k blocks into area #4000. »

**Confirmations indépendantes, mot pour mot.**

[CW-GA], « Register 3 - RAM Banking » :

> ```
>  -Address- 0     1     2     3     4     5     6     7
>  0000-3FFF RAM_0 RAM_0 RAM_4 RAM_0 RAM_0 RAM_0 RAM_0 RAM_0
>  4000-7FFF RAM_1 RAM_1 RAM_5 RAM_3 RAM_4 RAM_5 RAM_6 RAM_7
>  8000-BFFF RAM_2 RAM_2 RAM_6 RAM_2 RAM_2 RAM_2 RAM_2 RAM_2
>  C000-FFFF RAM_3 RAM_7 RAM_7 RAM_7 RAM_3 RAM_3 RAM_3 RAM_3
> ```
> « The Video RAM is always located in the first 64K, VRAM is in no way affected
> by this register. »

[GRIM-GA], « Regular RMR Register » (son `S` = bit 2, `MM` = bits 1-0, `p` =
page) :

| bits 2-0 | #0000-#3FFF | #4000-#7FFF | #8000-#BFFF | #C000-#FFFF |
|---|---|---|---|---|
| `0 00` | Bank 0 : Base 64Kb | Bank 1 : Base 64Kb | Bank 2 : Base 64Kb | Bank 3 : Base 64Kb |
| `0 01` | Bank 0 : Base 64Kb | Bank 1 : Base 64Kb | Bank 2 : Base 64Kb | Bank 3 : Page p |
| `0 10` | Bank 0 : Page p | Bank 1 : Page p | Bank 2 : Page p | Bank 3 : Page p |
| `0 11` | Bank 0 : Base 64Kb | **Bank 3 : Base 64Kb** | Bank 2 : Base 64Kb | Bank 3 : Page p |
| `1 bb` | Bank 0 : Base 64Kb | **Bank b : Page p** | Bank 2 : Base 64Kb | Bank 3 : Base 64Kb |

[CT-MEM] donne les mêmes 8 sélections sous forme de tables de vérité issues du
service manual (« `*` indicates a sub-block in the second 64k of ram ») :
sélection 0 → 0,1,2,3 ; 1 → 0,1,2,3\* ; 2 → 0\*,1\*,2\*,3\* ; 3 → 0,3,2,3\* ;
4 → 0,0\*,2,3 ; 5 → 0,1\*,2,3 ; 6 → 0,2\*,2,3 ; 7 → 0,3\*,2,3. Et son
observation : « **Only memory ranges &4000-&7fff and &c000-&ffff are effected by
memory paging.** » (vrai pour toutes les configurations sauf `010`).

[LOGON35], source d'époque, dit la même chose en langage naturel :

> « Le bit 2, lorsqu'il est à 1, permet de connecter le block dont le numéro est
> contenu par les bits 1 et 0, de #4000 à #7FFF. **Pour déconnecter ces blocks,
> il suffit de mettre 0 sur les bits 2, 1, 0 et la Ram redevient normalement
> linéaire.** Mais il existe un cas, des plus intéressants, lorsque le B2=0,
> B1=1, B0=0, il se trouve tout simplement (!!!) que les 4 blocks de 16 Ko, se
> connectent les uns après les autres de #0000 à #FFFF, pour former une nouvelle
> Ram de 64 Ko »

et le piège qui va avec, que le §7 de la spec ne mentionne pas :

> « vu que les Ram Plus ne sont pas connectées en vidéo (merci Amstrad !), tout
> ce qui sera chargé de #C000 à #FFFF ne sera pas visible, seule l'ancienne page
> écran sera affichée »

confirmé par [GRIM-GA] : « **The extended RAM can only be used by the CPU!** It
is not possible to use it as video RAM or for running a DMA-List! » et par
[CW-GA] : « The Video RAM is always located in the first 64K ». **Aucune
divergence entre les cinq sources sur la table des 8 configurations.**

**Comparaison mot pour mot avec la table du §7.**

| `ccc` | Formulation du §7 | Ce que disent les sources | Verdict |
|---|---|---|---|
| `000` | « aucune RAM étendue connectée » | RAM de base linéaire, blocs 0,1,2,3 — indépendamment de la présence d'une extension. [S968] : « the normal default bank layout, i.e. Banks 0..3 » ; [LOGON35] : « la Ram redevient normalement linéaire » | **réfuté** |
| `001` | « 4ᵉ page de la RAM étendue (page `ppp`) en `&C000` » | bloc 3 de la page `ppp` en `&C000`, le reste en RAM de base. Contenu juste ; « page » désigne ici un bloc de **16 K**, alors que `ppp` désigne une page de **64 K** dans le même document | **à nuancer** (terminologie) |
| `010` | « les 64 K entiers de la page étendue basculent dans l'espace adressable » | blocs 0,1,2,3 de la page `ppp` sur `&0000`-`&FFFF` | **confirmé** |
| `011` | « les 16 K habituellement en `&C000` passent en `&4000`, et la 4ᵉ page étendue en `&C000` » | bloc 3 **de base** en `&4000`, bloc 3 **de la page `ppp`** en `&C000` | **confirmé** |
| `100` | « 1ʳᵉ page du bloc de 64 K additionnel en `&4000`–`&7FFF` » | bloc 0 de la page `ppp` en `&4000` | **confirmé** (même flottement « page » / « bloc de 16 K ») |
| `101` | « idem avec la 2ᵉ page » | bloc 1 de la page `ppp` en `&4000` | **confirmé** |
| `110` | « idem avec la 3ᵉ page » | bloc 2 de la page `ppp` en `&4000` | **confirmé** |
| `111` | « idem avec la 4ᵉ page » | bloc 3 de la page `ppp` en `&4000` | **confirmé** |

Deux omissions du §7, sans erreur : (a) les configurations `001`, `011` et
`100`-`111` laissent `&0000`-`&3FFF` et `&8000`-`&BFFF` en RAM de base — ce que
le §7 laisse implicite ; (b) la RAM étendue n'est jamais visible du CRTC, donc
une section écran ne peut pas y vivre.

### C.3 — Nombre de bits de page réellement décodés (question 8)

| Extension | Bits de page décodés | Source |
|---|---|---|
| **64 K** (CPC 464/664 nu) | registre inexistant | [CT-GA] : « if this expansion is not present then the function is not available » |
| **128 K** (CPC6128, ou 464/664 + extension 64 K) | **aucun** — une seule page. [S968] impose bits 4 et 3 = 0, bit 5 réservé. [CW-GA] : « always 0 on an unexpanded CPC6128 » | [S968] app. E ; [CW-GA] |
| **jusqu'à 512 K** | **3 bits : D5-D3** (`ppp` = 0..7) | [CW-MEMEXP] : « 5-3 Bank number in 64K units (for expansions bigger than 64K) (**max = 512K**) » ; [GRIM-GA] : « The regular RMR register supports up to 8 pages, thus **512Kb** of extended RAM » ; [CW-GA] : « 0-7 on Standard Memory Expansions » |
| **au-delà de 512 K (1 M à 4 M)** | D5-D3 pour les bits de poids faible, **+ A10-A8 de l'adresse de port** pour les bits de poids fort, en général inversés | [CW-MEMEXP] ; [GRIM-GA] |

[CW-MEMEXP], mot pour mot, sur la méthode au-delà de 512 K :

> « Expansions bigger than 512K extend that above standard. The **LSBs of the 64K
> bank number is kept in D5-D3** data bits (as above), the **MSBs of the 64K bank
> number are in A10-A8 address bits. Typically in inverted form** (so the first
> 512K block is accessed via Port 7Fxxh, the next via 7Exxh, etc.
>
> `OUT [01111aaaxxxxxxxx],11bbbccc`
> aaa = upper bits of 64K bank selection (512K-step) (A8,A9,A10 bits)
> bbb = lower bits of 64K bank selection (64K-step) (D3,D4,D5 bits)
> ccc = configuration (as in CPC6128) »

et l'avertissement qui compte pour un linker :

> « Compatibility - The first 512K (Port 7Fxxh) are fully compatible with
> dk'tronics. Using the next 1.5MB (Port 7Exxh,7Dxxh,7Cxxh) should work without
> conflicting with the other hardware expansions. Compatibility Problems - **The
> last 2MB (Port 7Bxxh,7Axxh,79xxh,78xxh) can conflict with other hardware.** […]
> For example 7B01h or 7B7Fh would be no good, because the FDC command register
> at FB7Fh is mirrored to these locations. »

[GRIM-GA] concorde : « RAM expansions with more than 512Kb usually provide one,
or more, custom RMR registers (**located at differents I/O addresses, usually
&7E00, &7D00, etc**). One for each 512Kb block of additionnal memory. »

Deux pièges, non discutables et à écrire dans un profil de cible :

- Les numéros de page ne commencent pas toujours à 0. [CW-MEMEXP] : « **Caution -
  The 64K bank numbers don't always start at bank 0** (In the Dk'tronics 256K
  Silicon Disc, the 64K banks are numbered **4..7**). »
- Une extension externe désactive l'extension interne. [GRIM-GA] : « **Any
  external RAM expansion plugged to a 128K machine will automagically disable the
  built-in 64Kb RAM expansion!** eg. a 6128 with a 256Kb RAM expansion will have
  64Kb (it's base 64Kb RAM) + 256Kb of extended RAM ».

**Divergence d'époque.** [LOGON35] écrit : « **Les bits 4 et 3 ne sont utilisés
que pour les extensions 256 Ko.** » — deux bits de page seulement, donc 4 pages,
donc 256 K. Ce n'est pas faux pour le matériel de 1990 décrit par l'article, mais
c'est plus étroit que le standard dk'tronics 512 K documenté par [CW-MEMEXP] et
[GRIM-GA]. Autorité : [CW-MEMEXP] / [GRIM-GA], plus tardifs et plus complets ;
[LOGON35] est un instantané historique.

---

## D. CPC Plus

### D.1 — `RMR2` : disposition et fenêtres (question 9)

**Sélection.** `RMR2` est le même code de commande que `RMR` avec **bit 5 = 1**,
soit `%101xxxxx`, et uniquement ASIC déverrouillé. [GRIM-GA], tableau des
registres :

> | 7 | 6 | 5 | 4..0 | Machine | Register | Description | Chip |
> |---|---|---|---|---|---|---|---|
> | 1 | 0 | 0 | n | All | RMR | Control Interrupt counter, ROM mapping and video mode | Gate Array |
> | 1 | 0 | 1 | n | All | RMR | Ghost | Gate Array (CPC) or locked ASIC (Plus) |
> | 1 | 0 | 1 | n | Plus | RMR2 | ASIC & Advanced ROM mapping | Unlocked ASIC |

**Disposition et fenêtres.** [GRIM-GA], section « RMR2 », mot pour mot :

> « The RMR2 register extends the functionnalities of the RMR register about the
> Lower ROM mapping and also controls the ASIC I/O page mapping (between #4000 to
> #7FFF) as shown in the table below: »
>
> | bits 7-5 | LRM (bits 4-3) | ROM ID (bits 2-0) | #0000-#3FFF | #4000-#7FFF | #8000-#BFFF | #C000-#FFFF |
> |---|---|---|---|---|---|---|
> | `101` | `00` | P | **ROM P** | | | |
> | `101` | `01` | P | | **ROM P** | | |
> | `101` | `10` | P | | | **ROM P** | |
> | `101` | `11` | P | **ROM P** | **ASIC I/O** | | |

avec ses exemples : « `ld bc,&7FB8` ; `%101 11 000` » (page I/O ASIC en `&4000`,
ROM basse en `&0000`) et « `ld bc,&7FA0` ; `%101 00 000` ».

Donc `RMR2` = `101` + `LRM` (2 bits) + `ROM ID` (3 bits, une ROM **physique** de
cartouche, 0-7).

**Trois nuances que le §7 n'énonce pas, toutes tirées de [GRIM-GA] :**

1. Ce n'est pas un mécanisme parallèle à celui de la ROM haute : `RMR2`
   **déplace la ROM basse**, et reste **subordonné au bit `LR` de `RMR`** —
   « The physical ROM P is actually mapped in the CPU address space **only if the
   RMR.LR bit is clear (0 ⇒ mapping ON)**. So you can use RMR.LR=%1 and
   RMR2.LRM=%11 to enable only the ASIC I/O page mapping without any Lower ROM
   mapped anywhere. »
2. Les fenêtres possibles sont **quatre**, pas deux : `&0000`-`&3FFF` (défaut,
   `LRM=00`), `&4000`-`&7FFF` (`LRM=01`), `&8000`-`&BFFF` (`LRM=10`), et
   `LRM=11` qui remet la ROM en `&0000` **et** mappe la page I/O de l'ASIC en
   `&4000`. Le §7 ne cite que les deux du milieu.
3. Seules les **8 premières** ROM physiques de la cartouche sont adressables
   ainsi — « You can only map the first 8 physical ROMs of the cartdridge as
   Lower ROM. **To access physical ROM above 7, you have to use the Upper ROM
   mapping.** » Confirmé par [CW-UROM] : « The first 8 physical roms can be
   accessed as lower roms. And all the 32 physical roms can be accessed as upper
   roms. »

**Valeur par défaut**, [GRIM-GA] : « The default setting of the RMR2 register is
configured to page the Firmware ROM on the cartdridge from #0000 to #3FFF, just
like a CPC would do, to maintain software compatibility. »

**Le port `&DF00` change aussi de sémantique sur Plus.** [GRIM-GA] :

> | 7 | 6..5 | 4..0 | Description |
> |---|---|---|---|
> | 0 | L | L | « L is a logical ROM ID from 0 to 127 » |
> | 1 | x x | P | « P is a physical ROM ID on the cartdridge from 0 to 31 » |

[CW-UROM] dit exactement la même chose : « On Plus, this port behaves in 2 modes,
indicated by bit7: if bit7 = 0, it accepts a logical ROM number from 0-127 ; If
bit7 = 1, bits6..5 are ignored and bits4..0 is a physical ROM number from 0-31 ».
Aucune divergence.

### D.2 — Le déverrouillage ASIC est-il nécessaire ? (question 10)

**Oui.** Trois sources concordantes, mot pour mot :

- [GRIM-GA], en encadré au-dessus de la table `RMR2` : « **This register is only
  available on Plus machines and must be unlocked before being used** (See the
  ASIC documentation for the unlock sequence). »
- [GRIM-GA], à propos du bit 5 : « on a Plus machine with **it's RMR2 register
  unlocked**, this bit is used to select the RMR2 register. […] you should
  consider **locking the RMR2 register first**, just to make sure that if your
  CPC program end up running on a Plus, it won't select the RMR2 register
  unexpectedly ».
- [CT-GA] et [CW-GA] : « When the ASIC is "locked", the extra features are not
  available and the ASIC operates the same as the Gate-Array in the CPC allowing
  programs written for the CPC to work on the Plus without modification. **The
  ASIC must be "un-locked" to access the new features.** »
- [CT-CPCPLUS] (<https://cpctech.cpcwiki.de/docs/cpcplus.html>) : « Before the
  ASIC ram can be accessed, the ASIC must be enabled. **A special sequence must be
  programmed before the features are unlocked.** (See the official ASIC
  documentation for details of this.) »

**La séquence de déverrouillage elle-même n'a pas été relevée dans cette
recherche** : les deux sources renvoient à la documentation ASIC officielle, non
consultée. À traiter comme non trouvé plutôt que reconstitué.

Conséquence pratique pour un profil de cible : sur Plus, un programme qui
n'utilise pas `RMR2` doit garder **bit 5 = 0** dans toutes ses écritures `RMR`,
faute de quoi il touchera `RMR2` si l'ASIC se trouve déverrouillé.

---

## E. Verdict sur la spec

Reprise une à une des affirmations du §7 de `docs/spec-chaine-outils.md`.

### E.1 — Affirmation par affirmation

| # | Affirmation du §7 | Verdict | Formulation corrigée |
|---|---|---|---|
| 1 | `RMR` est sélectionné par `100` dans les bits 7-5, et il détermine quelles ROM sont visibles | **à nuancer** | Le registre est sélectionné par **bit7 = 1, bit6 = 0**. Le bit 5 est **réservé (envoyer 0)** et non un bit de sélection : sur CPC il est sans effet, sur **Plus ASIC déverrouillé** un 1 sélectionne `RMR2` ([S968] app. E ; [GRIM-GA]). Le §7 en pose la bonne valeur, pour une raison qu'il n'énonce pas. |
| 2 | Le registre s'écrit sur le port du Gate Array | **confirmé** (implicite dans le §7) | Adresse recommandée **&7Fxx** ; décodage réel **A15 = 0 et A14 = 1**, tous les autres bits d'adresse indifférents ([CT-GA], [GRIM-IO]). |
| 3 | Position des bits : `v` = bit 4, `R` = bit 3 (ROM haute), `r` = bit 2 (ROM basse), `mm` = bits 1-0 | **confirmé** | Positions exactes ([S968] app. E). |
| 4 | « `r` : **1 = ROM basse activée** » | **réfuté** | « `r` (bit 2) : **0 = ROM basse active ; 1 = ROM basse inhibée**, la RAM sous-jacente est lue. » [S968] : « Bit 2: Lower half ROM disable ». |
| 5 | « `R` : **1 = ROM haute activée** » | **réfuté** | « `R` (bit 3) : **0 = ROM haute active ; 1 = ROM haute inhibée**, la RAM sous-jacente est lue. » [S968] : « Bit 3: Upper half ROM disable » ; « the mode and ROM enable register is set to zero, **enabling** both halves of the ROM ». |
| 6 | « `v` : remise à 0 du **compteur vsync** (toujours 0 ici) » | **à nuancer** | Le compteur visé est le **diviseur par 52 du compteur de HSYNC** qui engendre les interruptions, pas un compteur de VSYNC ([CT-GAINT] ; [S968] « the divide by 52 counter used for generating periodic interrupts »). « Toujours 0 ici » est correct et cohérent avec la valeur au reset. **Ce que fait exactement le bit reste contradictoire entre sources** : [S968] dit qu'il n'efface que le bit de poids fort du compteur, [GRIM-GA] / [CT-GAINT] / [LOGON35] disent qu'il le remet entièrement à 0. Non tranché par mesure ici ; l'interprétation « remise à zéro complète » est retenue comme la mieux étayée. |
| 7 | « `mm` : mode écran » | **confirmé** | `00`/`01`/`10` = modes 0/1/2 ([S968]). `11` = mode 3, qui **existe** matériellement (160×200, 4 couleurs) mais que [S968] marque « **Do not use** ». Le changement ne prend effet qu'au HSYNC suivant. |
| 8 | « Sur CPC standard, le **numéro** de ROM haute est choisi par le port `&DF00` » | **confirmé** | Décodage réel : **A13 = 0** seul, adresse recommandée `&DFxx`, **écriture seule** ([CT-EXPROM], [GRIM-IO], [WMG]). Le numéro ne rend rien visible sans `RMR` bit 3 = 0. Si aucune ROM ne porte le numéro, c'est la ROM haute embarquée (BASIC) qui répond ([S968] §10). Plage : 0-255 côté matériel, 0-251 côté firmware. |
| 9 | La RAM est pilotée par un PAL, à la même adresse d'E/S que le Gate Array | **confirmé** | [CW-MEMEXP] nomme le composant (PAL16L8) ; [S968] §2.5 dit « ULA », terme moins précis. Décodage : **A15 = 0 seul** pour le PAL (contre A15 = 0 **et** A14 = 1 pour le Gate Array), et **écriture seule** ([GRIM-IO], [CT-IOPORD], [CW-MEMEXP]) — avec la contradiction interne de [CT-MEM] relevée en C.1, arbitrée en faveur de A15-seul. Le registre **n'existe pas** sur 464/664 sans extension. |
| 10 | Disposition `11pppccc`, `ppp` = numéro de page de 64 K, 0 à 7 | **confirmé** pour les extensions standard ; **à nuancer** pour le 6128 nu | `ppp` = bits 5-3, `ccc` = bits 2-0 ([CW-MEMEXP], [CW-GA], [GRIM-GA]). Sur un CPC6128 non étendu, `ppp` **doit valoir 0** — [S968] va jusqu'à spécifier « Bit 4: 0 / Bit 3: 0 ». `ppp` sur 3 bits couvre **512 K** ; au-delà, les bits de poids fort passent par **A10-A8 de l'adresse de port**, en général inversés (`&7Fxx`, `&7Exx`, …) ([CW-MEMEXP], [GRIM-GA]). Les numéros de page ne commencent pas toujours à 0 selon l'extension. |
| 11 | Table `ccc` : ligne `000` = « aucune RAM étendue connectée » | **réfuté** | « **RAM de base linéaire : blocs 0, 1, 2, 3 en `&0000`, `&4000`, `&8000`, `&C000`. Aucune banque étendue visible.** » La configuration ne dit rien de la *présence* d'une extension ; c'est simplement la disposition par défaut. [S968] : « Selecting Code 0 […] will switch to the normal default bank layout, i.e. Banks 0..3 ». Le symbole `__cfg_none` du §12.3, glosé « aucune extension connectée », porte la même erreur de formulation. |
| 12 | Table `ccc` : lignes `001`, `010`, `011`, `100`, `101`, `110`, `111` | **confirmé** quant au contenu ; **à nuancer** quant aux mots | Le mot « page » désigne dans ces lignes un **bloc de 16 K**, alors que le même §7 emploie `ppp` pour une **page de 64 K**. Écrire « bloc » ou « banque de 16 K » pour les uns, « page de 64 K » pour l'autre. Manque par ailleurs : dans `001`, `011` et `100`-`111`, `&0000`-`&3FFF` et `&8000`-`&BFFF` restent en RAM **de base** ; et la RAM étendue **n'est jamais visible du CRTC**, donc aucune section écran ne peut y résider ([GRIM-GA], [CW-GA], [LOGON35]). |
| 13 | « Sur CPC Plus, `RMR2` étend le mécanisme et permet de mapper des ROMs sur `&4000`–`&7FFF` et `&8000`–`&BFFF` » | **à nuancer** | Ces deux fenêtres sont **exactes** (`LRM=01` et `LRM=10`) mais la description est incomplète et légèrement fausse dans son cadrage. `RMR2` **redirige la ROM basse**, pas la ROM haute ; il reste **subordonné à `RMR` bit 2 = 0** ; il offre **quatre** dispositions (dont `&0000`-`&3FFF` par défaut, et une qui mappe la page I/O de l'ASIC en `&4000`) ; il ne désigne que les **8 premières ROM physiques** de la cartouche ; et il **exige le déverrouillage de l'ASIC** ([GRIM-GA], [CW-UROM], [CT-GA]). |
| 14 | Le déverrouillage ASIC (non mentionné par le §7) | **non trouvé** (la séquence) | L'**exigence** de déverrouillage est confirmée par quatre sources. La **séquence** de déverrouillage elle-même n'a pas été relevée : [GRIM-GA] et [CT-CPCPLUS] renvoient à la documentation ASIC officielle, non consultée ici. À vérifier avant d'écrire quoi que ce soit dans un profil Plus. |

### E.2 — La polarité des bits de ROM fait-elle l'objet de formulations contradictoires ?

**Non.** Sept sources indépendantes, réparties sur les quatre niveaux
d'autorité — [S968] (Amstrad), [GRIM-GA], [CT-GA], [CT-EXPROM], [LOGON35] (en
français), [WMG] (ouvrage imprimé), [CW-GA] — énoncent **toutes** que **1
inhibe et 0 active**, pour la ROM basse comme pour la ROM haute ; aucune source
consultée n'affirme le contraire, ni sous forme de tableau, ni sous forme
d'exemple de code. Le §12.3 se trompe donc en présentant cette polarité comme
« précisément le genre d'information sur laquelle les sources de documentation se
contredisent » : sur ce point, la littérature est unanime, et c'est **le §7 qui
est seul à dire l'inverse**.

Ce qui *ressemble* à une contradiction, et explique probablement l'erreur, est un
piège de **nommage, non de valeur** : [S968] intitule le registre « MODE AND ROM
**ENABLE** REGISTER » tout en nommant ses bits « ROM **disable** ». Un lecteur
rapide de « ROM enable register » peut en déduire « 1 = enable ». La phrase de
[S968] « the mode and ROM enable register is set to zero, enabling both halves of
the ROM » lève l'ambiguïté sans appel.

L'argument de fond du §12.3 — inscrire la polarité **une fois** dans un profil de
cible plutôt que la recopier dans chaque source — reste entier ; seul l'exemple
choisi pour le justifier est mal choisi. Les points où la littérature se
contredit réellement, et qui mériteraient ce traitement, sont ailleurs :

1. **L'effet exact du bit 4** (`RMR`) : effacement du seul bit de poids fort du
   diviseur ([S968]) contre remise à zéro complète du compteur ([GRIM-GA],
   [CT-GAINT], [LOGON35]). Non tranché ici. Voir A.3.
2. **Le décodage du port du PAL** : A15 = 0 seul ([GRIM-IO], [CT-IOPORD],
   [CW-MEMEXP]) contre A15 = 0 **et** A14 = 1 ([CT-MEM], qui contredit
   [CT-IOPORD] du même auteur). Sans conséquence si l'on écrit sur `&7F00`. Voir
   C.1.
3. **Le décodage du port du Gate Array selon [WMG]**, qui omet A14 = 1 et décrit
   en réalité le masque du PAL. Voir A.1.
4. **Le nombre de bits de page** : 2 bits / 256 K ([LOGON35], d'époque) contre
   3 bits / 512 K ([CW-MEMEXP], [GRIM-GA]), contre 0 bit sur un 6128 nu
   ([S968]). Voir C.3.
