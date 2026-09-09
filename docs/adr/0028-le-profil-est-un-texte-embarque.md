---
status: accepted
---

# Le profil de cible est un texte embarqué, lu par l'analyseur du script

L'étage C1 a donné à fantams le **placement calculé** : la fenêtre donne son
adresse à une section, la configuration donne sa banque, et le profil dit sous
quel numéro cette banque se range. Une source peut désormais ne nommer aucun
emplacement, et déplacer une section ne touche pas une de ses lignes.

Le profil de cible est le fichier de données sur lequel tout cela repose. Cet
ADR fixe **quel objet il est**, parce que c'est la décision que la prochaine
personne à ouvrir le code ne pourra pas reconstituer.

## Contexte

Le §9 de `spec-chaine-outils.md` décide que « les profils de cible sont compilés
dans le binaire », et que `--target cpc6128` nomme une carte mémoire intégrée. Il
ne dit pas **sous quelle forme**, et le §13 pose par ailleurs que « décrire un ZX
ou un MSX ne doit pas demander de toucher au code du linker » — sans que rien ne
porte cette affirmation.

Deux dessins s'offraient. Une `struct` C++ littérale, avec un analyseur ajouté
plus tard pour les profils de l'utilisateur. Ou un **texte**, embarqué, lu par le
même analyseur que celui d'un profil de l'utilisateur.

## Décision

**Le profil livré EST un fichier de profil** : un texte, embarqué dans le
binaire, lu par l'analyseur qui lit aussi `-P mien.prof`. Un porteur, un
analyseur, un chemin de code, **un jeu de valeurs**.

Trois raisons, dans l'ordre de leur poids.

**1. Deux porteurs pour les mêmes valeurs.** Le §7 s'est pris cette faute dans la
figure et en a tiré sa règle : *une valeur écrite deux fois est une valeur fausse
une fois.* Un profil en `struct` **plus** un profil en texte est exactement cette
faute, appliquée aux valeurs les moins vérifiables du projet.

**2. L'export serait un second sérialiseur.** `--dump-profile` devrait écrire du
texte depuis une `struct`, et ce sérialiseur peut diverger de l'analyseur — la
faute précise que l'étape B7 a testée pour le `.fo` par un aller-retour de
chaînes. Sous cette décision, l'export est une **copie**, et le test est une
comparaison d'octets. Il n'existe aucun écrivain qui pourrait mentir.

**3. Un sérialiseur perdrait les citations, qui sont la valeur.** Dans un profil,
les commentaires portent « sept sources concordantes », « non tranché par
mesure », « mesuré sur tel modèle à telle date ». Le §12.3 fait de cette
distinction *par valeur* le cœur de son argument. Un profil exporté sans sa
provenance est un profil que **personne ne peut auditer**.

Et le coût qui semblait interdire ce dessin n'existe pas : **l'analyseur de
script est nécessaire de toute façon**, et les deux langages partagent leur
lexeur et leur grammaire à blocs. Le profil n'est pas un second analyseur — c'est
un second jeu de mots-clés sur le même, et il n'est né qu'au moment où il avait
un second consommateur.

## Ce qui en découle, et qui n'aurait pas été gratuit autrement

**`-P` est livré tout de suite.** `--target` et `-P` sont le même chemin de
code : cacher le second ne ferait rien gagner, et l'exposer **prouve** que le
texte embarqué n'a aucun privilège. `tests/accept_profile.sh` le tient — le
profil livré, sa copie exportée et *aucun profil* rendent le même octet.

**L'invariant du §13.2 devient vérifiable.** `tests/no_machine_names.sh` interdit
tout nom de machine dans le linker et ses deux langages, et **nomme son
périmètre** : `profiles.cpp` est exempt parce qu'il est une donnée et non du
code ; le builder est dehors, un format de snapshot connaissant sa machine à bon
droit ; les tests sont dehors, leur travail étant de nommer des machines. Il a
mordu au premier passage, sur un commentaire qui citait un registre — ce qui est
le meilleur argument pour l'avoir écrit **avec** le code, comme le §13.2 le
demandait.

**Les valeurs matérielles ont un seul domicile.** Le profil livré porte les huit
configurations de la RAM, la polarité des bits de ROM et les deux ports, chacune
marquée `ATTESTE` ou `NON TRANCHE`, et renvoyant au dossier de recherche qui les
arbitre. Les trois contradictions que ce dossier laisse ouvertes sont **dans le
texte du profil**, là où elles comptent, et non en note de bas de page.

## Deux amendements que ce dessin a imposés

Ils sont écrits dans `spec-chaine-outils.md`, à l'endroit qu'ils corrigent, et
non ici : une décision écrite à la fin d'un étage est une décision que personne
n'a lue au moment où elle comptait.

**Le §6 ne dit plus `RMR.BIT2 = (on ? 0 : 1)`, mais `MASK … , CODE << 2`.**
L'ancienne forme mettait le nom d'un registre d'une machine **dans la
grammaire** — ce que l'invariant ci-dessus interdit — et elle disait deux choses
à la fois, alors que le §12.3 exige déjà les deux séparément sous les noms
`__mask_<axe>` et `__val_<axe>_<config>`. `MASK` n'est pas une invention : c'est
le mot qui manquait à `__mask_`.

**Le §6 ne dit plus `on { w3 rom_hi<n> }, ROM 15`, mais `on<n>` et
`CONFIG rom_upper.on<15>`.** Le paramètre appartient à l'**état**, comme
`ext_w1<b>` le montrait déjà ; un `<n>` que rien ne lie est refusé dans le
profil, et un qualificatif que rien ne consomme est refusé dans le script.

## Une addition au vocabulaire : `STORE`

Une banque nommée doit devenir l'entier que `--sym` imprime déjà dans sa colonne
`store`, que le dump plat emploie et que le refus au-delà de la banque 7 lit.
C'est la **numérotation de la machine elle-même** — les sources primaires
numérotent ses blocs de 0 à 7 —, et elle est donc déclarée dans le profil.

Le déduire de l'ordre des lignes aurait rendu l'ordre du fichier sémantique :
déplacer deux lignes aurait changé chaque `.sym` et la disposition de chaque
snapshot **sans un mot**. Et une plage de banques reçoit une **plage** de
numéros — `STORE 0..3` —, parce qu'un seul numéro pour quatre banques aurait été
une attribution consécutive implicite, et l'implicite est ce que `STORE` existe
pour retirer.

## Une seconde décision : les symboles de commutation sont des CONSTANTES

Le §12.3 écrit `ld bc, __port_ram_audio | __val_ram_audio`, et cette composition
n'est possible que si les deux termes sont des **nombres**. Un `EXTERN` ne le
permet pas : il déclare une **adresse**, deux inconnues ne se composent pas dans
une relocalisation qui n'en porte qu'une, et une adresse ne tient pas dans un
octet.

*Amendement.* Cet ADR citait la somme — `__port_ram_audio + __val_ram_audio` —,
et l'argument ne change pas d'un opérateur à l'autre : `|` exige les mêmes
nombres, plus strictement encore, puisque `+` est le seul opérateur que
l'assembleur laisse relocalisable. Ce qui a changé est la raison d'écrire `|` :
une somme rend un nombre vraisemblable quand l'octet bas du port n'est pas nul —
un `POKE` en donne un —, là où la composition dit ce qu'elle fait.

**Ces symboles ne dépendent d'aucune adresse.** Un port, une valeur bornée aux
bits de son axe et un masque se calculent dès que l'état, son argument et sa
banque sont connus — donc dès que le script et le profil sont là, avant qu'un
octet soit assemblé. Le CLI les calcule et les passe à l'assembleur comme des
**constantes**.

Aucun mécanisme nouveau, et trois propriétés tenues :

- **l'assembleur ne connaît toujours aucune machine.** Il reçoit des chiffres,
  comme un compilateur C reçoit ses `-D`. Le §1 est intact ;
- **le calcul est une fonction pure**, `link::switchSymbols(script, profile)`, et
  le linker l'appelle par le même chemin pour ses `EXTERN`. Une fonction, donc
  une valeur : la faute de deux porteurs ne peut pas se produire ici non plus ;
- **la compilation séparée reste possible.** Une unité assemblée sans script les
  déclare par `EXTERN` et le linker les résout, au prix de l'arithmétique dans
  cette unité-là. Et l'unité qui commute est précisément, par le §13.4, le seul
  morceau non portable d'un programme — celle qu'on assemble avec sa carte.

`__off_<section>` est la seule exception, et sa raison est dans sa définition :
un offset est une adresse moins une base, donc il dépend du placement. Il se
résout au linkage.

## Ce que cet ADR ne décide pas

La **compression** — sortie de C1, mécanisme et ordre écrits, algorithme non. La
**vérification** — continuité et ses trois pointeurs, sections miroir, accord
structurel, `CLOBBERS`, `INIT_FROM`, co-visibilité des références entre états —
est C2 tout entière. Le **builder** — snapshot à chunks, `.dsk`, `.cpr`, `.cro`,
et la signature plate de `sna::build` — reste hors des étages numérotés. Et les
**sources de vérification sur machine**, qui transformeraient « non tranché par
mesure » en valeur mesurée, sont écrites comme une étape autonome et non faites.
