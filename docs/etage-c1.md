# Étage C1 — suivi

> **Ce fichier tient lieu de ticket.** Une ligne d'état par étape, mise à jour au
> fur et à mesure ; il n'y a rien d'autre à synchroniser. Le *quoi* est dans
> [spec-etage-c1.md](spec-etage-c1.md), le *où* dans
> [coutures-de-la-chaine.md](coutures-de-la-chaine.md) ; ici, l'ordre, les arêtes
> et l'état.

**Ce qu'est l'étage C1**, d'après le §10 : le linker qui **place et calcule** —
fenêtres, banques, configurations, `ORG` déduit, symboles de commutation,
chevauchements inter-sections. Ce qu'un programme banqué devient constructible.

**Ce qu'il n'est pas.** Il ne **vérifie** rien de ce que C2 vérifiera : ni la
continuité et ses trois pointeurs, ni les sections miroir, ni `CLOBBERS`, ni
`INIT_FROM`, ni la co-visibilité des références (D10). Et il ne compresse pas
(D9) : le mécanisme et l'ordre forcé sont écrits, l'algorithme non. C'est ce qui
permet de livrer C1 sans promettre C2.

**L'appel de conception de l'étage** est que **trois étapes se testent seules,
avant qu'aucun octet ne bouge** : les deux analyseurs sont des fonctions pures
que rien ne traverse, et la fusion par nom est un changement de placement pur,
testable avec des objets fabriqués à la main sans un mot de profil.

**Les deux invariants qui fixent l'ordre** (D13) :

1. le dépôt compile et les **dix** suites sont vertes à chaque étape — les neuf
   d'origine plus `accept_separate` ;
2. **le cas simple ne paie rien** à chaque étape (D12) : sans script, sans
   `--target` et sans `-P`, les octets, les binaires d'`examples/` et les
   diagnostics sont ceux d'aujourd'hui.

**Sur la numérotation.** Les étapes s'appellent `C1.0` à `C1.10` et non `C0` à
`C10` : l'étage s'appelle déjà C1, et `C1` désignerait alors deux choses. Les
étapes de C2 s'appelleront `C2.0`, `C2.1`, et ainsi de suite.

---

## État

| # | Étape | Bloqué par | État |
|---|-------|-----------|------|
| C1.0 | La section fusionnée par nom à travers les objets *(préfacteur)* | — | à faire |
| C1.1 | L'analyseur de script : syntaxe et diagnostics, sans résolution | — | à faire |
| C1.2 | Le langage de profil, le CPC embarqué, le lexeur extrait | C1.1 | à faire |
| C1.3 | `--target`, `-P`, `--dump-profile` | C1.2 | à faire |
| C1.4 | L'`ORG` déduit : la fenêtre place la section | C1.0, C1.2 | à faire |
| C1.5 | Le chevauchement inter-sections, et le mou chiffré | C1.4 | à faire |
| C1.6 | `OFFSET` / `SIZE` : découper une banque au placement | C1.4 | à faire |
| C1.7 | Les symboles de commutation, `bank()` et `BankOf` | C1.2, C1.4 | à faire |
| C1.8 | `__off_`, `__romnum_`, et les refus de `COMPRESS` / `MIRROR` | C1.7 | à faire |
| C1.9 | L'exemple d'acceptation du §12.2 | C1.3, C1.5, C1.6, C1.8 | à faire |
| C1.10 | L'ADR de clôture, et l'ADR 0005 relu | tout | à faire |
| C1.V | Les sources de vérification sur machine réelle *(autonome)* | — | à faire |

Trois étapes ne sont pas dans la chaîne : **C1.0 et C1.1 sont parallèles**, et
**C1.V ne dépend de rien et ne débloque rien** — comme A6, elle est hors du
chemin critique et peut se faire à tout moment.

Fin de l'étage C1 : C1.0 à C1.10 faites, les dix suites vertes plus celle du
placement calculé, `docs/syntax.md` à jour, et l'exemple d'acceptation de C1.9
dont le quatrième contrôle passe — déplacer une section **dans le script seul**
change sa banque et sa valeur de commutation, et pas une adresse logique.

---

## C1.0 — la section fusionnée par nom *(préfacteur)*

**Ce qu'il livre.** Rien de visible sur un programme d'un seul objet, et c'est
l'énoncé. Sur un programme multi-objet, une section de même nom cesse de recevoir
deux bases disjointes : elle devient **une entité du linkage** dont les fragments
viennent de N objets.

**Pourquoi d'abord.** C'est ce qui rend C1.4 petit — la fenêtre donnera sa base à
*une* section, pas à une section par objet — et c'est le seul gain du §11
(remplir une ROM avec les sections `"ro"` de dix fichiers) qui ne demande aucun
profil. Même rôle que B0 : fabriquer l'objet avant d'y toucher.

- [ ] Les sections de même nom de N objets ne reçoivent qu'une base
- [ ] Leurs fragments s'y suivent dans l'ordre où les objets sont donnés
- [ ] Le type est figé par la première déclaration **à travers les objets**, et rouvrir en `"rw"` ce qu'un autre a déclaré `"ro"` est refusé en nommant les deux unités
- [ ] Deux plafonds différents pour un même nom : refusés, en nommant les deux sites — retenir le plus petit serait défendable, et c'est la raison de refuser
- [ ] Un test épingle l'ordre pour **deux objets déclarant chacun deux sections** : `a1 a2 b1 b2`, et non `a1 b1 a2 b2` comme aujourd'hui
- [ ] `accept_separate` reste vert
- [ ] Les dix suites vertes, et D12 tient

## C1.1 — l'analyseur de script

**Ce qu'il livre.** Un texte → une valeur `Script`, et des diagnostics qui
nomment leur ligne. **Aucune résolution** : un script qui parle d'une
configuration que le profil ne porte pas passe l'analyse sans un mot — c'est
C1.4 qui le refusera, parce que c'est là que le profil existe.

**Pourquoi ici.** C'est la couture **réelle** de l'étage : le script est écrit par
l'auteur. Et il se teste seul, sans profil, sans objet et sans un octet.

- [ ] `TARGET`, `MEMORY_MAP`, `CONFIG <nom>[<param>]`, `w<n> { SECTION <nom> … }`
- [ ] `w<n> [OFFSET x, SIZE y] { … }` est **analysé** ici, employé en C1.6
- [ ] `OUTPUT_FORMAT { TARGET, ENTRY_POINT, STACK, INT_VECTOR }` — analysé et porté ; `STACK` et `INT_VECTOR` ne servent qu'à C2, et un champ analysé mais non lu est préférable à un champ que C2 devra rétro-insérer
- [ ] Un mot-clé inconnu est **refusé**, jamais ignoré
- [ ] `COMPRESS` et `MIRROR` sont reconnus et refusés en nommant l'étage (les messages définitifs sont en C1.8)
- [ ] Se teste seul : texte → `Script`, une suite à part entière
- [ ] Les dix suites vertes, et aucune sortie ne change — rien ne lit encore un script

## C1.2 — le langage de profil, le CPC embarqué, le lexeur extrait

**Ce qu'il livre.** Le vocabulaire du §13.1, dans les limites de D2 ; le profil
CPC 6128 + RAM128 du §13.2 **écrit en texte et embarqué dans le binaire**, avec
ses valeurs prises dans `docs/recherche/` et ses citations en commentaires ; et
le lexeur partagé, extrait **maintenant** parce que c'est maintenant qu'il a deux
consommateurs.

- [ ] `WINDOW`, y compris plusieurs grilles superposées
- [ ] `BANK … SIZE …`, `ro` / `rw`, `VIDEO`, `CONTENDED` — les trois porteurs du §13.1
- [ ] `CONFIG SET`, `CONFIG`, `OVER`, et la forme paramétrique `ext_w1<b> [CODE %1bb]`
- [ ] `SELECT <axe> = OUT|POKE <port>, <valeur>` — **les nombres seulement**
- [ ] Les mots de C2 — `STACK OUTSIDE`, fenêtre d'exécution interdite, `LOCKS`, séquences, préconditions — sont refusés **en nommant l'étage C2** (D2)
- [ ] `SHADOWS` et `ALWAYS` ne sont pas des mots du langage
- [ ] Refusés : une fenêtre déclarée deux fois, une banque sans `SIZE`, une configuration nommant une banque inconnue, un axe sans `SELECT`
- [ ] Le lexeur et la grammaire à blocs sont **partagés** avec C1.1, et l'extraction ne change pas un diagnostic de script
- [ ] Le profil CPC porte, par valeur, ce qui est *attesté par la documentation* et ce qui est *non tranché* — les trois contradictions du §12.3 sont visibles dans le texte
- [ ] Se teste seul : texte → `Profile`

## C1.3 — `--target`, `-P`, `--dump-profile`

**Ce qu'il livre.** Les trois formes de D3, et la preuve que le profil embarqué
n'a aucun privilège.

- [ ] `--target cpc6128` nomme le texte embarqué
- [ ] `-P mien.prof` le remplace, **par le même chemin de code**
- [ ] `--dump-profile cpc6128` rend le texte embarqué, et un test le compare **octet pour octet** — le seul contrôle qui attrape la divergence que D1 rend impossible
- [ ] Un `--target` inconnu est refusé en listant les profils embarqués
- [ ] Sans `--target` ni `-P` : aucun profil, et le placement de l'étage B (D12)

## C1.4 — l'`ORG` déduit

**Ce qu'il livre.** Le cœur de l'étage. `placeRelocSections()` cesse d'être un
curseur : la fenêtre où la configuration fait apparaître la banque donne sa base
à la section, et la banque de rangement vient de la configuration.

`build` prend ses deux entrées de plus (D6) ; `Image` ne gagne pas un champ.

- [ ] `Image build(objects, script, profile)` ; `Image`, `Block`, `Symbol` et `flatten` inchangés
- [ ] Un script vide et un profil vide donnent **exactement** le placement de l'étage B, et un test le dit
- [ ] Une section nommée dans `CONFIG c { wN { SECTION s } }` est basée à l'adresse de `wN`, et rangée dans la banque que `c` donne à `wN`
- [ ] L'`ORG` vient de la fenêtre de **la grille à laquelle appartient la banque** (D5), et un test le pose sur deux grilles superposées
- [ ] Refusés, en nommant les trois : une configuration inconnue, une fenêtre que la configuration ne concerne pas, une section que nul objet ne porte
- [ ] Une section `"uninit"` occupe la place sans émettre un octet
- [ ] Une section qui dépasse sa banque est refusée **avec le dépassement chiffré**
- [ ] Une section relocalisable que le script ne place pas retombe sur le placement dérivable du §9
- [ ] Le placement passe par le refus au-delà de la banque 7 (D11), qui existe déjà
- [ ] Les dix suites vertes, et D12 tient

## C1.5 — le chevauchement inter-sections, et le mou

**Ce qu'il livre.** Le refus que seul un placement calculé peut prononcer, et le
chiffre dont l'auteur a besoin pour arbitrer.

- [ ] Deux sections qui se disputent des octets dans une banque : **refus**, en nommant les deux sections et la banque
- [ ] Deux sections dans deux grilles superposées dont les fenêtres se recouvrent dans un même état : même refus, **calculé** et non déclaré (§13.1)
- [ ] Le mou de chaque banque employée est signalé et chiffré : `2048 bytes unused at 0xE000 in rom_hi15`
- [ ] Le refus inter-objets de l'étage B n'est pas doublé : deux diagnostics pour un seul fait en valent zéro
- [ ] Se teste avec des objets fabriqués à la main, sans un source Z80

## C1.6 — `OFFSET` / `SIZE`

**Ce qu'il livre.** Le découpage de placement **à l'intérieur** d'une banque, et
non une banque plus petite (§13.1).

- [ ] `w3 [OFFSET 0x0000, SIZE 0x2000]` et `w3 [OFFSET 0x2000, SIZE 0x2000]` cohabitent dans une banque de 16 K
- [ ] Plusieurs sections dans le même bloc s'y concatènent, dans l'ordre du script
- [ ] Un débordement du `SIZE` déclaré est refusé, chiffré
- [ ] Deux blocs qui se recouvrent dans la même banque sont refusés
- [ ] Ce n'est pas une banque de 8 K : les deux moitiés apparaissent ensemble ou pas du tout, et un test le pose

## C1.7 — les symboles de commutation, `bank()` et `BankOf`

**Ce qu'il livre.** Le triplet par axe de D7, et la fonction qui manquait à
l'assembleur.

- [ ] `__port_<axe>_<config>`, `__val_<axe>_<config>`, `__mask_<axe>`
- [ ] Un `EXTERN` sur un de ces noms se résout **sans qu'aucun objet ne l'exporte**
- [ ] La valeur est **bornée aux bits de l'axe** — jamais l'octet global (D7)
- [ ] Les noms préfixés de `__` sont réservés : une source qui en définit un est refusée (ADR 0015)
- [ ] `bank(label)` — nouvelle fonction, sur le modèle de `high()` / `low()` de l'ADR 0027 — produit une relocalisation `BankOf`
- [ ] Le langage sait exprimer un port **fonction de la banque** (§13.1) ; le profil CPC de C1 ne l'emploie pas, et un profil de test l'exerce
- [ ] `beautify` connaît `bank`
- [ ] `docs/syntax.md` : `bank()`, et les noms `__` comme réservés

## C1.8 — `__off_`, `__romnum_`, et les deux refus

- [ ] `__off_<section>` : l'offset dans sa banque
- [ ] `__romnum_<nom>` : le numéro de ROM haute — la **seconde** écriture d'un `SELECT` qui en compte deux
- [ ] `COMPRESS` dans un script : refusé **en nommant l'enveloppe du §8** — `SECTION blob, "ro", 0x2000` —, pas en nommant un étage futur
- [ ] `MIRROR` dans un script : refusé en nommant C2
- [ ] L'ordre forcé *placer → compresser → résoudre* et le refus de non-convergence sont **écrits** dans la spec de C1, et vérifiés le jour où le compresseur arrive (D9)

## C1.9 — l'exemple d'acceptation du §12.2

**Ce qu'il livre.** La preuve que le renversement a eu lieu. L'exemple du §12.2
**sans sa section compressée** (D9) : quatre sections, un profil, un script de
dix lignes.

- [ ] `examples/banked.asm` (ou son découpage) s'assemble, se linke et s'exporte en SNA 128 K
- [ ] `main` en banque 1, `sysbank` en banque 2, `audio` en banque 5
- [ ] `audio_init` vaut `&4000 + offset` **sans qu'un `org` l'ait dit**
- [ ] `__val_ram_audio` = `&C5`, `__val_ram_linear` = `&C0`, comparés dans le test
- [ ] **Le contrôle qui compte** : déplacer `audio` d'`ext1` vers `ext2` **dans le script seul** change la banque de rangement et la valeur de commutation, et **pas une adresse logique**
- [ ] Le script est inscrit dans les **deux** listes de tests, `Makefile` et `CMakeLists.txt`

## C1.10 — l'ADR de clôture, et l'ADR 0005 relu

- [ ] ADR : le profil est un **texte embarqué**, un porteur, un analyseur, un export qui est une copie — avec les trois raisons de D1
- [ ] L'ADR 0005 : son statut est **écrit**. `org b<n>:` reste licite ; la question qu'il tranchait ne se pose plus au niveau du linker
- [ ] Le tableau du §10 de `spec-chaine-outils.md` : la compression sort de C1 (D9)
- [ ] `docs/syntax.md` à jour : le script, le profil, `bank()`, les noms `__`
- [ ] Les décisions prises en cours de route sont dans **l'étape qui les a provoquées**, pas ici

## C1.V — les sources de vérification sur machine réelle *(autonome)*

**Ce qu'il livre.** Ce que le §12.3 reconnaît ne pas avoir : « vérifiée une fois,
sur machine ou sur émulateur » est son argument central, et **rien ne l'incarne**.
Les valeurs de `docs/recherche/` sont vérifiées sur *documentation*, un cran en
dessous, et trois contradictions y sont laissées ouvertes.

Une source **minuscule et autonome par valeur litigieuse**. Elle ne se compare
pas à des octets attendus — son juge est la machine. Elle rend un résultat
observable, et son verdict remonte dans le dossier de recherche avec sa **date**
et le **modèle exact** employé.

- [ ] L'effet du **bit 4 de `RMR`** : le manuel Amstrad dit « le bit de poids fort du diviseur », quatre autres sources disent « le compteur entier »
- [ ] Le **décodage du port du PAL** : A15 = 0 seul, ou A15 = 0 **et** A14 = 1
- [ ] Le **nombre de bits de page réellement décodés** : 2, 3, ou zéro sur un 6128 nu
- [ ] Chaque verdict remonte dans `docs/recherche/cpc-gate-array-rmr.md`, daté et attribué à un modèle
- [ ] Le profil de C1.2 distingue alors, **par valeur**, *attesté par la documentation* de *mesuré ici, sur telle machine, à telle date*

**Ce que ça change.** « Non tranché par mesure dans le cadre de cette
recherche » — la phrase revient trois fois dans les dossiers — devient une dette
nommée, ou disparaît.
