# Spécification : étage E — le second profil, et les constantes de profil (`CONST`)

## Le problème

`docs/spec-etage-c1.md` §Hors périmètre écartait délibérément « un deuxième
profil livré » de C1, pour ne pas payer « la conception de trois machines pour
n'en livrer aucune » (§13.2). Mais trois questions concrètes, listées dans
`docs/etage-c1.md` §C1.7 et la mémoire de chantier
`port-et-valeur-a-revoir-au-second-profil`, ne pouvaient pas se trancher avec
un seul profil sur la table :

- le nom d'axe dans la clé des symboles de commutation (`__val_ram_audio` vs
  `__val_audio`) ;
- le rang de `__port_<axe>_<clé>` — un symbole par axe, alors qu'il ne varie
  jamais sur CPC ;
- si le profil devait déclarer des constantes de matériel (`GA_PORT`), à la
  place du compromis « port en dur » retenu en C1.7.

Le CPC+ est ce second profil. `docs/recherche/cpc-gate-array-rmr.md` §D
l'a déjà tranché sur sources primaires : la RAM est identique au 6128 (même
PAL, même port, même table `ccc`), et ce qui diffère réellement est un axe
neuf — `RMR2`, qui redirige la ROM basse et mappe la page E/S de l'ASIC — sur
**le même port matériel**, `0x7F00`.

## La solution

`docs/adr/0032-…` tranche ce que le CPC+ permet de trancher, et dit
explicitement ce qu'il ne permet pas :

1. `CONST <nom> = <expr>` entre dans le langage de profil : une constante,
   calculée au chargement du profil, passée au source comme les symboles de
   commutation. `GA_PORT = 0x7F00` remplace le triplet `__port_` sur les deux
   profils CPC.
2. L'axe sort de la clé de `__val_` (mais pas de `__mask_` : trouvé en
   préparant E2, voir la correction dans l'ADR — `__mask_<axe>` n'a aucune clé
   de section pour le remplacer). Argument structurel pour `__val_` : une
   section n'appartient jamais qu'à un seul axe. Pas empirique — le CPC+ n'a
   fait que fournir l'occasion de l'écrire.
3. Le rang général de `__port_` (« partout un invariant par cible ? ») **reste
   ouvert** : `RMR2` est décodé par le même composant que `RMR`, donc le CPC+
   ne teste rien qui soit hors de la famille CPC. Écrit dans l'ADR pour ne pas
   être retranché ailleurs par erreur.

Ce que l'étage livre en plus, au-delà de l'ADR : le profil CPC+ lui-même, et
les deux points de langage (`CONST`, renommage des clés) qui le rendent
écrivable sans compromis.

## Ce qu'il n'est pas

Il ne livre **aucun conteneur** : ni `.cpr`, ni le builder du §3.3 de
`spec-chaine-outils.md`. C'est la dette nommée par l'ADR 0027, et cet étage ne
la solde pas — il prépare seulement le profil dont un futur builder CPR aura
besoin (fenêtres de `RMR2`, numérotation des ROM physiques). Il ne modélise
pas le verrou ASIC : `docs/recherche/cpc-gate-array-rmr.md` §D.2 explique que
c'est un état d'exécution, imperceptible au linkage. Il ne touche pas à
l'axe RAM, identique au 6128 et déjà attesté.

## Décisions d'implémentation

### E1 — `CONST` est une constante de profil, au même chemin que les symboles de commutation

Analysée par le même lexeur/grammaire que le reste du profil (§D1 de
`spec-etage-c1.md` : un profil est un texte, lu par l'analyseur du script).
Le CLI la calcule dès le profil chargé — avant tout placement, puisqu'elle ne
dépend d'aucune section — et la passe à l'assembleur comme une constante
numérique, exactement comme `link::switchSymbols` le fait déjà pour
`__port_`/`__val_`/`__mask_`. `--dump-profile` la réimprime : c'est une copie,
pas un second sérialiseur (même raison que D1 de C1).

Pas de conflit de nom à arbitrer dans cette étape : un profil qui déclare deux
`CONST` du même nom est refusé, en nommant les deux lignes — même règle que
les plafonds divergents de C1.0.

### E2 — L'axe sort de `__val_` ; `__port_`, `__romnum_` ET `__mask_` restent inchangés

`__val_<axe>_<clé>` devient `__val_<clé>` — `<clé>` est un nom de section,
toujours unique dans le programme lié, donc jamais ambigu une fois l'axe
retiré. `__mask_<axe>`/`__mask2_<axe>` gardent leur axe : ils n'ont **aucune**
clé de section (`link.cpp::offer("__mask_" + axisName, mask)`, rien d'autre),
et le retirer collapserait plusieurs masques distincts du même profil vers un
seul nom nu. `__port_` n'est pas retiré du langage : un profil dont le port
varierait par axe continue de l'émettre. Sur CPC et CPC+, plus aucune source
n'a besoin d'y toucher une fois `GA_PORT` adopté — mais le mécanisme reste
disponible, et un test dédié (`accept_const.sh`) vérifie que `__port_`
apparaît toujours quand un profil de test ne déclare pas `CONST`.

### E3 — Le profil CPC+, écrit sur `docs/recherche/cpc-gate-array-rmr.md` §D

Fenêtres et banques RAM : **identiques au CPC 6128**, copiées et non
réinventées — le §D.1 confirme la même table `ccc`. `rom_lower`/`rom_upper`
copiés aussi : `RMR` ne change pas sur Plus. Un axe neuf, `cart_rom` (`RMR2`,
sélectionné par les bits 7-5 = `101`, redirige la ROM **basse** — ni `lrom2`
ni la forme à quatre états esquissées plus haut : livré tel qu'écrit ici) :

```
BANK crom<n> SIZE 0x4000 ro STORE 16

CONFIG SET cart_rom OVER ram {
    w0<n> [CODE %00000 | n] { w0 crom<n> }
    w1<n> [CODE %01000 | n] { w1 crom<n> }
    w2<n> [CODE %10000 | n] { w2 crom<n> }
}
SELECT cart_rom = OUT GA_PORT, %10100000 | CODE
```

`CODE = (LRM << 3) | n` compose en un seul octet la fenêtre (`LRM`, 2 bits)
et l'ID de ROM physique de la cartouche (`n`, 3 bits, 0..7 — §D.1 point 3 :
seules les huit premières ROM physiques sont adressables ainsi). Vérifié
contre les deux exemples littéraux de [GRIM-GA] : `w0<0>` = `%101 00 000` =
`&7FA0`, et la disposition `LRM=11` (non écrite ici) = `%101 11 000` =
`&7FB8`. Aucun `MASK` : comme pour l'axe `ram`, les bits 7-5 = `101`
n'appartiennent qu'à ce registre. `STORE 16` n'est **pas** l'ID physique —
c'est un numéro d'emplacement de cet outil, déplacé hors de 0..9 déjà pris ;
l'ID physique attesté reste `PAGE`, comme pour `rom_hi<n>` (C1.8).

**Hors périmètre, nommé dans le profil et non subi** — trois choses
attestées par §D, non écrites :
- `LRM = 11` (la ROM reste en `w0`, **et** la page E/S de l'ASIC apparaît en
  `w1`) : une page d'E/S n'est pas une banque adressable par une `SECTION`,
  la représenter demanderait un mot de vocabulaire de profil que rien
  d'autre ne consomme encore ;
- les ROM physiques 8..31 de la cartouche, adressables seulement en ROM
  **haute** (même port `&DF00` que `rom_upper`, mais un ID physique et non
  le numéro logique de `rom_hi<n>`) ;
- le déverrouillage de l'ASIC — état d'exécution, jamais représentable comme
  un `SELECT` (ADR 0032, décision 3 et `cpc-gate-array-rmr.md` §D.2).

Chaque valeur porte son statut, comme `profiles.cpp` l'exige déjà : `ATTESTE`
pour ce que §D.1 cite mot pour mot ; et un `NON TRANCHE` hérité du 6128 sans
changement (le bit 4 de `RMR`, `RMR` étant le même registre) — pas propre au
Plus, mais le profil ne peut pas prétendre l'avoir résolu en le passant sous
silence.

### E4 — Critère de fin d'étage

Livré dans `tests/link_test.cpp`, sur le profil **livré** (pas un profil de
test fabriqué à la main) : `--dump-profile cpcplus` se relit sans erreur ;
`GA_PORT` s'y résout à `0x7F00` ; la RAM y calcule les mêmes octets qu'au
6128 (`__val_audio` = `&C5`, même fixture) ; `cart_rom.w0<3>` calcule `&A3`,
et le déplacer vers `w1<3>` dans le script **seul** donne `&AB` — la fenêtre
a changé, pas l'ID physique de ROM (même preuve que C1.9, portée à un axe
neuf).

## Hors périmètre

Le builder et tout conteneur (`.cpr`, `.dsk`, `.cro`) — ADR 0027, non soldée
ici. La ROM haute du CPC+ et ses 32 ROM physiques. La séquence de
déverrouillage de l'ASIC — état d'exécution, jamais représenté dans un profil
(ADR 0032). Les macros de profil, un cran au-dessus d'`CONST` — réservées
tant qu'un troisième profil n'a pas montré qu'il en fallait. Le rang général
de `__port_` hors famille CPC — ouvert jusqu'à un profil ZX Next ou MSX.
