---
status: accepted
---

# La ROM CRO se déduit de son emplacement

Le format CRO (Logon System, RIFF `CRO `) décrit chaque ROM par un type
(RTYP), un slot (RLOG) et un numéro physique (RPHY), rangée dans un groupe de
ROMs (GRRO). fantams ne demande **aucun** de ces champs au source ni à
l'invocation : ils se déduisent de la banque que l'image remplit et du numéro
passé par `--cro-rom <n>`, comme le nom d'un morceau se déduit de son
emplacement (ADR 0007). Un même binaire donne donc toujours la même ROM CRO.

## Ce qui est décidé

| Banque du profil      | RTYP                         | RLOG (slot)          | RPHY | RID    |
| --------------------- | ---------------------------- | -------------------- | ---- | ------ |
| `rom_lo` (STORE 8)    | `LOW` (0)                    | 0                    | 0    | `lo`   |
| `rom_hi<n>` (STORE 9) | `HIGH` (1)                   | `n`                  | `n`  | `hiNN` |
| `crom<n>` (STORE 16)  | `BANKABLE` (2) si `n < 8`, sinon `HIGH` (1) | 7 si `n == 3`, sinon 1 | `n`  | `cbNN` |

`NN` est décimal sur deux chiffres, comme dans les fichiers produits par
CROMANAGER.

- **Sur l'ancienne gamme, le numéro physique est le slot.** Le support n'y a pas
  de rang propre ; une carte range pourtant la ROM à base + RPHY × 16 K. Prendre
  le slot plutôt que le rang d'arrivée dans le groupe rend le résultat
  indépendant de l'ordre des invocations, au prix de « trous » que le guide CRO
  prévoit déjà (ROM vide).
- **Le type d'une ROM de cartouche suit la limite matérielle.** L'image ne dit
  pas quel axe (`cart_rom` ou `cart_rom_hi`) a placé la banque ; mais seules les
  ROMs physiques 0 à 7 sont mappables en bas, et le seuil 8 est exactement cette
  limite. Le slot 7 → physique 3 reproduit la table de transposition du Plus.
- **Un groupe est d'une seule famille** — cartouche, ou classique (`rom_lo` et
  `rom_hi<n>` ensemble). Sinon la ROM haute 7 et `crom7` se disputeraient le
  même numéro physique, et un groupe est ce qu'une carte sélectionne d'un geste.
  Dans un groupe, une ROM est identifiée par son numéro physique **et** par le
  fait d'être la ROM basse (RTYP `LOW`) : `lo` et `hi00` ont tous deux le
  numéro 0 et coexistent, une carte les distinguant par leur type. La famille
  d'une ROM déjà rangée se lit à son **RID** (`lo`/`hiNN` classique, `cbNN`
  cartouche), jamais à son RTYP — `HIGH` vaut pour les deux. Une ROM au RID
  inconnu, venue d'un autre outil, est neutre : elle ne fixe pas la famille du
  groupe, et se remplace comme les autres sur son numéro physique.
- **Le masque n'est jamais déduit** : `0xFFFFFFFF` sauf `--cro-mask` (ADR 0013).

## Écarts assumés avec la spécification écrite

La spécification et les fichiers réels divergent ; fantams suit les fichiers
(les exemples du dépôt et l'export de CROMANAGER) :

- la taille d'un chunk est celle de ses données, sans « moins 8 » — RIFF
  standard, avec un octet de padding après une taille impaire ;
- GNUM, GLBL et GMSK sont toujours **écrits**, dans cet ordre, avant les ROMs ;
  un groupe **relu** sans eux reçoit les défauts du guide (numéro d'ordre,
  masque `0xFFFFFFFF`) ;
- les chunks inconnus d'un fichier fusionné sont conservés à leur place.

## Options écartées

- RPHY = rang dans le groupe : dépend de l'ordre des invocations.
- RPHY ou RTYP fournis par drapeau : un champ de plus à se tromper, pour une
  valeur que l'emplacement donne déjà.
