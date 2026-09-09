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
2. L'axe sort de la clé de `__val_`/`__mask_` : argument structurel (une
   section n'appartient jamais qu'à un seul axe), pas empirique — le CPC+ n'a
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

### E2 — L'axe sort de `__val_`/`__mask_` ; `__port_` et `__romnum_` restent inchangés

`__val_<axe>_<clé>` devient `__val_<clé>`, `__mask_<axe>` devient `__mask` —
uniquement quand l'axe est nommé seul seraient ambigus autrement, même règle
de retrait que celle qui existe déjà pour les états (`[axe.]état`,
`script.h`). `__port_` n'est pas retiré du langage : un profil dont le port
varierait par axe continue de l'émettre. Sur CPC et CPC+, plus aucune source
n'a besoin d'y toucher une fois `GA_PORT` adopté — mais le mécanisme reste
disponible, et un test dédié (`accept_const.sh`) vérifie que `__port_`
apparaît toujours quand un profil de test ne déclare pas `CONST`.

### E3 — Le profil CPC+, écrit sur `docs/recherche/cpc-gate-array-rmr.md` §D

Fenêtres et banques RAM : **identiques au CPC 6128**, copiées et non
réinventées — le §D.1 confirme la même table `ccc`. Un axe neuf, `lrom2` (nom
provisoire — RMR2 redirige la ROM **basse**, sur quatre dispositions, la
quatrième mappant en plus la page E/S de l'ASIC) :

```
CONFIG SET lrom2 OVER ram {
    default [CODE %00] { w0 rom_lo }
    mid     [CODE %01] { w1 rom_lo }
    high    [CODE %10] { w2 rom_lo }
    io      [CODE %11] { w0 rom_lo  w1 asic_io }
}
SELECT lrom2 = OUT GA_PORT, MASK %01100000, %101_00000 | (CODE << 3)
```

(la forme exacte de l'écriture — comment `%101` se compose avec le sélecteur
de registre et `LRM` — se règle à l'implémentation, sur le tableau exact du
§D.1 ; ce qui compte ici est que l'axe existe, recouvre `ram` comme
`rom_lower`/`rom_upper` le font déjà, et n'invente aucun port). `STORE` pour
les huit premières ROM physiques adressables en ROM basse (§D.1, point 3) ;
les 32 ROM physiques totales, adressables seulement en ROM haute, restent hors
périmètre — aucun axe de ROM haute n'est écrit pour le CPC+ dans cet étage.

Chaque valeur porte son statut, comme `profiles.cpp` l'exige déjà : `ATTESTE`
pour ce que §D.1 cite mot pour mot, et rien de `NON TRANCHE` puisque
`cpc-gate-array-rmr.md` §D ne laisse aucune contradiction ouverte sur cet axe
(contrairement à `RMR` sur 6128 nu, qui en laisse trois).

### E4 — Critère de fin d'étage

`--dump-profile cpcplus` contre le texte embarqué (même contrôle que D1 de
C1). Un exemple d'acceptation : une section RAM (identique en octets à
l'exemple C1 du §12.2, prouvant que l'axe `ram` n'a pas changé) et une section
`ro` placée sous `lrom2.mid`, avec deux contrôles — `GA_PORT` vaut `0x7F00`
sur les deux profils, sans qu'aucune source ne l'écrive deux fois ; et changer
`lrom2.mid` en `lrom2.high` dans le script **seul** déplace la fenêtre, pas
une adresse logique (même preuve que C1.9, portée à un axe neuf).

## Hors périmètre

Le builder et tout conteneur (`.cpr`, `.dsk`, `.cro`) — ADR 0027, non soldée
ici. La ROM haute du CPC+ et ses 32 ROM physiques. La séquence de
déverrouillage de l'ASIC — état d'exécution, jamais représenté dans un profil
(ADR 0032). Les macros de profil, un cran au-dessus d'`CONST` — réservées
tant qu'un troisième profil n'a pas montré qu'il en fallait. Le rang général
de `__port_` hors famille CPC — ouvert jusqu'à un profil ZX Next ou MSX.
