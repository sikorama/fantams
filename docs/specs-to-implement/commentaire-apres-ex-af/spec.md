# Spec : un commentaire après `ex af,af'`

Status: ready-for-agent

## Problem Statement

Un auteur qui écrit `ex af,af' ; échange des registres` voit l'assembleur
refuser la ligne. La même ligne sans commentaire passe, et `ex de,hl ; …` aussi.

Cause vérifiée : la fonction qui repère le début d'un commentaire (partagée par
l'assembleur, le préprocesseur et la mise en forme) lit l'apostrophe de `af'`
comme l'ouverture d'une chaîne. Le `;` qui suit est donc « dans la chaîne » : il
n'est pas vu comme un commentaire, la ligne reste telle quelle et la
canonicalisation de `EX` ne la reconnaît plus. Le préprocesseur en porte la
trace : il retire le commentaire de `ld a,1 ; un` mais le laisse sur
`ex af,af' ; c`.

## Solution

L'apostrophe qui nomme le registre fantôme `af'` n'ouvre jamais une chaîne. Un
commentaire (`;` ou `//`) après `ex af,af'` est reconnu comme n'importe quel
autre, à tous les étages : l'assembleur accepte la ligne, le préprocesseur
retire le commentaire, la mise en forme l'aligne.

Les littéraux à apostrophes (`'A'`, `';'`, `'hello'-'a'`, ADR 0010) gardent
leur comportement, y compris quand ils contiennent un `;` ou un `//`.

## User Stories

1. En tant qu'auteur d'un programme CPC, je veux écrire `ex af,af' ; commentaire`,
   pour documenter l'échange de registres comme sur toute autre instruction.
2. En tant qu'auteur, je veux que `ex af,af' // commentaire` (style C) soit
   accepté de même, pour ne pas avoir à me souvenir d'une exception.
3. En tant qu'auteur, je veux que `EX AF,AF'` en majuscules, ou avec des espaces
   autour de la virgule, accepte aussi un commentaire, pour que la casse et la
   mise en page ne changent pas la règle.
4. En tant qu'auteur, je veux qu'une étiquette devant l'instruction
   (`swap: ex af,af' ; …`) n'y change rien.
5. En tant qu'auteur, je veux que `ex af,af'` seul, sans commentaire, se comporte
   comme avant, pour ne pas voir de régression.
6. En tant qu'auteur, je veux que `ex af,af ; commentaire` (apostrophe oubliée)
   soit toujours tolérée et canonisée en `ex af,af'`, avec son avertissement du
   préprocesseur, pour que le commentaire ne masque pas la tolérance de l'ADR 0020.
7. En tant qu'auteur, je veux que `ld a,';' ; point-virgule` garde son littéral
   `;` et son vrai commentaire, pour que le correctif ne casse pas les chaînes.
8. En tant qu'auteur, je veux que `db 'a;b' ; commentaire` reste lu comme une
   chaîne de trois octets suivie d'un commentaire.
9. En tant qu'auteur, je veux que `db "it's" ; commentaire` reste lu comme une
   chaîne, l'apostrophe y étant un caractère ordinaire.
10. En tant qu'utilisateur de la source déroulée, je veux que `ex af,af' ; c` en
    sorte comme les autres lignes (commentaire retiré ou conservé selon l'option),
    pour qu'elle reste réassemblable à l'identique (ADR 0017).
11. En tant qu'utilisateur de la mise en forme, je veux que le commentaire de
    `ex af,af'` s'aligne sur la colonne des commentaires des lignes voisines.
12. En tant que mainteneur, je veux que la règle « `af'` n'est pas une chaîne »
    vive en un seul endroit, pour que l'assembleur, le préprocesseur et la mise
    en forme ne divergent pas.
13. En tant que mainteneur, je veux que le cas soit couvert par un test qui
    échoue avant le correctif, pour éviter qu'il ne revienne.

## Implementation Decisions

- La correction se fait dans la fonction de repérage du commentaire, partagée
  par les trois étages ; aucun étage ne reçoit de contournement local.
- Règle : une apostrophe est la marque du registre fantôme, et non l'ouverture
  d'une chaîne, quand elle suit immédiatement le mot `AF` (insensible à la casse)
  pris comme mot entier — c'est-à-dire non précédé d'un caractère d'identifiant.
  Toute autre apostrophe ouvre une chaîne, comme aujourd'hui. Une règle plus
  large (« une apostrophe collée à un identifiant n'ouvre pas de chaîne ») est
  écartée : elle changerait la lecture de formes que l'ADR 0010 permet.
- La canonicalisation de `EX` (ADR 0020) n'est pas modifiée : une fois le
  commentaire correctement séparé, elle reconnaît la ligne.
- Les autres parcours de ligne qui suivent les guillemets (découpe sur `:`, sur
  `,`, recherche d'`=`, etc.) ont la même logique d'ouverture de chaîne. Le
  correctif les sonde avec `ex af,af'` ; ceux qui se trompent à leur tour sont
  corrigés par la même règle, de préférence en la partageant, sans élargir la
  règle elle-même. Ce qui reste hors de portée est consigné dans
  `improvements.md`.
- Aucune syntaxe nouvelle ; la doc de syntaxe n'a pas à changer sauf si un
  passage décrit `af'` comme interdisant le commentaire (à vérifier, et à
  corriger sans historique).

## Testing Decisions

- Un bon test ici n'observe que le comportement externe : source en entrée,
  octets émis (assembleur) ou texte de la source déroulée (préprocesseur) en sortie.
- Deux joints existants, aucun nouveau :
  - le test de l'assembleur seul (sans préprocesseur), qui a déjà
    « ex af,af' avec commentaire » en échec et son témoin `ex de,hl ; …` — à
    compléter des variantes des user stories 2, 3, 4, 6, 7, 8, 9 ;
  - le test du préprocesseur, qui porte déjà les cas `ex af,af` / `ex af,af'`
    et leur avertissement — à compléter par la story 10 (commentaire retiré
    ou conservé comme sur une ligne ordinaire).
- La mise en forme (story 11) se vérifie par son test existant s'il y en a un ;
  sinon par une sortie du préprocesseur, sans créer de joint.
- Ordre : le test existant doit échouer avant le correctif, passer après.
- Tests ciblés seulement pendant le travail (`make test-fast` ou le binaire de
  test concerné) ; `make test` une fois, au commit.

## Out of Scope

- Toute nouvelle tolérance de syntaxe, tout changement de l'avertissement de
  `ex af,af`.
- Les littéraux de caractère : leur règle (ADR 0010) ne bouge pas.
- Les deux hypothèses non reproduites déjà consignées dans `improvements.md`
  (avertissement parasite sans `run`, morceau de 64 K).

## Further Notes

- Le point correspondant dans `improvements.md` se retire quand la correction
  est faite.
- Bug signalé par un utilisateur, reproduit par un test déjà présent dans
  l'arbre de travail (non commité).
