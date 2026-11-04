---
status: accepted
---

# Pipeline d'export en trois couches, et noms de morceaux dérivés

L'export se décompose en trois couches composables plutôt qu'en un backend
monolithique par format :

```
image → découpage → morceaux → encapsulation → conteneur → artefacts
```

Un **morceau** porte `{ banque, adresse, point d'entrée, octets }`. Son nom
n'est pas déclaré mais dérivé.

La couche est `pack` (`pack.h`), le **builder**, et `package(image,
destination)` son entrée. Tous les formats de sortie y passent, avec le même
contrat : une image et une destination qui entrent, des artefacts, des
diagnostics et un bilan qui sortent, sans que rien ne touche au disque. La
profondeur, elle, dépend du format : seul le DSK passe par le découpage et
l'encapsulation.

## Contexte

Les formats visés — binaire brut, binaire préfixé AMSDOS, DSK, SNA, CPR, CRO —
partagent leurs constituants bien plus qu'il n'y paraît. Le découpage est commun
au binaire brut, au binaire AMSDOS et au DSK. L'en-tête AMSDOS de 128 octets est
commun au binaire AMSDOS et au DSK. Écrire DSK comme un bloc autonome reviendrait
à réimplémenter les deux.

C'est précisément par là qu'un assembleur dérive : chaque format finissant par
avoir ses propres options de découpage, mutuellement incohérentes. Composé, le
coût est d'un découpage plus une encapsulation plus un conteneur par format ;
écrit à la main, c'est un comportement par couple (format, découpage).

Le DSK cesse alors d'être un format pour devenir une composition, et « binaire
AMSDOS sur disquette » vient sans effort supplémentaire.

## Le découpage

Un morceau par emplacement d'un seul tenant. Le linker pose un bloc par
fragment, et coupe en plus un fragment des 64 K de base à chaque frontière de
16 K, parce que la banque y suit l'adresse. Le découpage trie les blocs par
emplacement et réunit deux blocs contigus en rangement quand ils sont dans le
même espace : la même banque d'extension, ou tous deux dans les 64 K de base à
leur fenêtre naturelle.

Sans cette réunion, un programme d'un seul tenant de `&4000` à `&9000`
deviendrait deux fichiers, et deux sections voisines d'une même banque étendue
aussi.

Les autres découpages envisagés — un morceau englobant tout, un par banque de
16 K, un par groupe de 64 K — et l'option qui choisirait entre eux attendent un
besoin réel.

## Ce qui ne passe pas par le découpage

Le SNA n'est pas un conteneur de morceaux mais un état machine : il lui faut la
mémoire entière plus des registres, une palette et un état CRTC, que `sna.cpp`
fabrique. Il prend donc l'image entière, posée sur une base s'il y en a une.
Reconnaître cette asymétrie coûte moins cher que d'inventer un découpage « tout
en un » pour le faire ressembler aux autres.

Le CPR et le CRO prennent la banque de ROM entière que l'image remplit, complétée
par des zéros : une ROM se livre d'un bloc, et la découper en morceaux pour les
recoller aussitôt n'apporterait rien. Une image ne porte qu'une banque, le
numéro physique n'y figurant pas (ADR 0033) : il entre par la destination.

Le binaire brut prend l'intervalle contigu que le linker produit déjà.

Le jour où le SNA transportera ses banques en chunks `MEM0`, `MEM1`… de la
version 3 (au-delà de 128 K, ADR 0006), leur compression RLE sera facultative —
un lecteur accepte les deux. Un piège à ne pas manquer : l'encodage est
`0xE5 <compte> <valeur>`, et un octet `0xE5` littéral doit être échappé en
`0xE5 0x00`, sur deux octets et sans troisième. Cette exception est vérifiée
empiriquement par la fonction `decodeRLE` du harnais de comparaison de
z80live-lite.

## Nommage

Un morceau **seul** prend le nom du source principal (le premier fichier
d'entrée), réduit à ce qu'AMSDOS accepte : sans chemin ni extension, en
majuscules, `A-Z 0-9 _ -`, 8 caractères au plus. C'est le cas courant — un
`org`, un `run` — et `RUN"DEMO"` vaut mieux que `RUN"B02_8000"`.

Dès qu'il y a plusieurs morceaux, ou si le nom du source ne garde aucun
caractère valable, le nom est dérivé du couple (banque, adresse) du morceau, et
non « de l'`ORG` » : plusieurs `ORG` peuvent tomber dans le même morceau, dont
l'adresse est celle de son début. Deux morceaux ne peuvent alors porter le même
nom que s'ils occupent le même emplacement, ce que le linker refuse déjà comme
recouvrement.

```
DSK :  B05_4000.BIN    B + banque en 2 chiffres hexa + _ + adresse en 4
                       = 8 caractères, la limite AMSDOS exactement
```

Dériver plutôt que déclarer découle de l'ADR 0004 : `SAVE "file.bin", start, len`
décrit une livraison, pas un programme. Et le nom porte l'information qui compte à
l'exécution — où charger le bloc — plutôt qu'une étiquette arbitraire.

## Conséquences

**Le contrat de backend n'est pas celui de `sna::build()`**, qui rend un
`vector<uint8_t>` unique : `package` rend un ensemble d'artefacts nommés, avec
ses diagnostics. Le fond de l'ADR 0002 — aucun chargement dynamique — n'est pas
touché.

**`package` ne touche pas au disque.** Ce que la destination doit savoir de
l'existant — une disquette, une cartouche ou un conteneur de ROMs à compléter,
une base — entre comme donnée, et c'est l'appelant qui écrit les artefacts.

Sur un DSK, les huit caractères d'un nom dérivé sont saturés : il ne reste aucune
place pour un préfixe. Le seul degré de liberté restant est l'extension, sur trois
caractères. La question ne se pose pas tant qu'un seul programme en banques
occupe une disquette, et sera tranchée quand elle se présentera — mais c'est là,
et nulle part ailleurs, que la place existe.

Le CRO introduit une notion qu'aucun autre format n'a : une arborescence, donc des
chemins. C'est pourquoi un artefact porte un chemin et non un nom.
