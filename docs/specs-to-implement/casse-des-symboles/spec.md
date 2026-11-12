Status: ready-for-agent

# Casse des symboles : sensibles, sans exception ni repli

Cadrage : journal « 2026-11-12 cadrage casse-des-symboles » (coffre, Projets/fantams).
Décision : ADR 0037 (paragraphe « Un symbole se résout à la lettre près »), ADR 0039.
Vocabulaire : `CONTEXT.md` (Symbole, Module, Source déroulée).

## Problem Statement

Un auteur de source fantams apprend que les symboles sont sensibles à la casse :
`Foo` et `foo` sont deux labels. Mais deux endroits trahissent cette règle.

1. Les macros (et les structures) se retrouvent sans tenir compte de la casse :
   `macro SCREEN_LINE()` et une variable `screen_line = 2` sont la même chose
   pour le préprocesseur. L'auteur obtient `macro 'SCREEN_LINE': expected 0
   argument(s), got 1` sur une simple affectation, sans comprendre pourquoi.
2. L'assembleur, lorsqu'un symbole est introuvable, retombe sur un symbole qui ne
   diffère que par la casse et n'émet qu'un avertissement. Une faute de casse
   produit donc un binaire, là où la règle annoncée voudrait une erreur.

Il n'existe aucune justification écrite de ces exceptions.

## Solution

Un symbole se résout à la lettre près, casse comprise, sans repli. Label,
constante, variable, macro, structure, module et paramètre sont sensibles à la
casse. Un nom introuvable à la casse écrite est une erreur qui propose la graphie
existante (`did you mean 'Foo'?`). Les mots du langage (mnémoniques, registres,
directives, mots-clés du préprocesseur) ignorent la casse et restent réservés
quelle que soit la casse. Aucune source n'est ménagée.

## User Stories

1. En tant qu'auteur, je veux que `Mm()` et `MM()` désignent deux macros
   distinctes, afin que les macros suivent la règle des labels.
2. En tant qu'auteur, je veux pouvoir écrire `screen_line = 2` à côté d'une
   `macro SCREEN_LINE()`, afin qu'une variable et une macro ne se confondent pas
   sur la seule casse.
3. En tant qu'auteur, je veux que `foo()` appelant une macro définie `Foo` soit
   une erreur nommée `unknown macro 'foo' — did you mean 'Foo'?`, afin de corriger
   la faute en un coup d'œil.
4. En tant qu'auteur, je veux que `ld a,foo` avec un label défini `Foo` soit une
   erreur `unknown symbol 'foo' — did you mean 'Foo'?`, afin qu'une faute de casse
   ne produise jamais de binaire.
5. En tant qu'auteur, je veux que deux symboles qui ne diffèrent que par la casse
   coexistent sans avertissement, afin que la règle n'ait pas de cas particulier.
6. En tant qu'auteur, je veux que deux structures `S` et `s` soient distinctes,
   et que leurs instances, tailles (`sizeof`) et champs suivent la casse écrite.
7. En tant qu'auteur, je veux qu'un module `Gfx` et un module `gfx` soient
   distincts, afin que `Gfx.plot` et `gfx.plot` ne se confondent pas.
8. En tant qu'auteur, je veux que `@Nom` ancre à la racine un symbole dont la
   casse est exacte, afin que l'ancre obéisse à la même règle.
9. En tant qu'auteur, je veux que la macro d'un module se retrouve par son nom
   qualifié à la casse exacte (`m.a()`), et que la suggestion de nom qualifié de
   l'ADR 0039 reste produite.
10. En tant qu'auteur, je veux que le paramètre `x` et le paramètre `X` d'une
    macro soient deux paramètres, afin que `{x}` et `{X}` ne s'écrasent pas.
11. En tant qu'auteur, je veux que `Nop`, `Ld` ou `ORG` restent refusés comme
    nom de label, de macro ou de constante, afin qu'un nom réservé le reste dans
    toute casse (ADR 0015).
12. En tant qu'auteur, je veux que `LD`, `ld` et `Ld` restent trois écritures de
    la même instruction, afin que les mots du langage ignorent la casse.
13. En tant qu'auteur, je veux que les compteurs d'expansion de macro
    (`m__1`, `m__2`) soient par nom exact, afin que `Mm` et `MM` aient chacune
    leur numérotation.
14. En tant qu'auteur, je veux que le `--beautify` ne traite comme appel de macro
    que le nom écrit à la casse exacte de la définition, afin que la mise en forme
    ne devine pas une macro là où il n'y en a pas.
15. En tant qu'auteur, je veux que `--timing` retrouve un label à la casse exacte
    seulement, afin que le rapport de durées obéisse à la même règle.
16. En tant qu'auteur, je veux que `-E` écrive des macros expansées sous la casse
    exacte de leur définition, afin que la source déroulée se réassemble.
17. En tant que mainteneur, je veux que l'ADR 0037 et `CONTEXT.md` décrivent le
    comportement réel après l'implémentation, afin qu'aucune note ne contredise
    le code.

## Implementation Decisions

- **Règle unique** (ADR 0037) : résolution exacte, casse comprise, pour tous les
  symboles ; aucun repli. Le texte de l'ADR est la référence.
- **Préprocesseur** : les tables des macros et des structures sont indexées par le
  nom exact (avec son préfixe de module), non plus par le nom en majuscules.
  Les recherches (appel de macro, `@nom`, recherche par module englobant, `sizeof`,
  instance de structure, redéfinition) passent par la même clé exacte.
- **Nom d'appel mis en majuscules** : au site d'appel, le nom de la macro parvient
  à la recherche déjà converti en majuscules (constaté par un essai : la table
  exacte ne le retrouve plus). Cette conversion est à localiser et à supprimer
  pour les noms de symboles ; elle doit rester pour les mots du langage.
- **Compteurs d'expansion** : clés exactes (nom qualifié de la macro ou mot-clé
  de boucle, ce dernier restant un mot du langage).
- **Table des macros utilisée par la mise en forme** : mêmes noms exacts que la
  table du préprocesseur.
- **Assembleur** : l'index insensible à la casse est supprimé en entier. Cela
  retire le repli avec avertissement pour les symboles et celui de la recherche
  de timelines ; les deux deviennent une recherche exacte. Un symbole introuvable
  est une erreur `unknown symbol` qui ajoute la graphie existante, s'il y en a une
  qui ne diffère que par la casse, sous la forme `did you mean 'X'?`. La
  suggestion ne résout jamais le nom.
- **Message côté préprocesseur** : `unknown macro 'foo' — did you mean 'Foo'?`,
  même forme que la suggestion de nom qualifié de l'ADR 0039 ; les deux
  suggestions peuvent se cumuler ou se choisir, une seule étant donnée par erreur.
- **Mots du langage** : inchangés. Insensibles à la casse, réservés dans toute
  casse. Une macro ou un label portant un mot réservé reste refusé.
- **Aucune transition** : pas d'avertissement préalable, pas d'option de
  compatibilité (ADR 0025).
- **Documentation** : le commentaire de `pp.cpp` qui dit « clé = nom majuscule »
  et celui de l'assembleur sur le repli insensible à la casse sont supprimés avec
  le code qu'ils décrivent. Le tableau « Command line » et les autres `.md`
  n'ont rien à changer, sauf tout passage qui annoncerait une insensibilité des
  symboles à la casse (à chercher et à signaler à la fin).

## Testing Decisions

Un bon test ne regarde que le comportement externe : une source en entrée, le
texte déroulé, les octets ou les diagnostics en sortie. Aucun test n'inspecte les
tables internes.

Deux seams existants, aucun nouveau (confirmé avec l'utilisateur) :

- **`tests/pp_test.cpp`** (`chk`, source → texte déroulé) : macros `Mm`/`MM`
  distinctes ; `foo()` face à `Foo` en erreur avec suggestion ; variable
  `screen_line` à côté de `macro SCREEN_LINE` ; structures `S`/`s`, `sizeof`,
  instances ; modules `Gfx`/`gfx` ; macro de module à la casse exacte et
  suggestion de nom qualifié conservée ; `@Nom` ; paramètres `x`/`X` ;
  compteurs d'expansion séparés ; mots réservés refusés en toute casse ; mots du
  langage acceptés en toute casse.
- **`tests/asm_test.cpp`** (`chk`, `chkErrHas`, `chkWarn`) : `ld a,foo` face à
  `Foo` en erreur avec `did you mean 'Foo'?` (et plus d'avertissement) ; `Foo` et
  `foo` coexistants sans avertissement ; constantes `equ` à la casse exacte.
- **Mise en forme** : le test existant du `--beautify` face aux noms de macros
  (`tests/beautify_test.cpp`) reçoit un cas de casse différente.
- **Timing** : un cas dans le test existant de `--timing` sur la recherche d'un
  label à la casse exacte.

Antécédents : les tests de macros de modules (ADR 0039) et les tests d'erreurs
nommées avec suggestion fournissent la forme à suivre. Les tests existants qui
définissent et appellent une macro dans deux casses différentes doivent être
corrigés (la casse de l'appel suit celle de la définition) ; l'essai préliminaire
n'en a pas trouvé dans les `.asm` du dépôt, mais pp_test.cpp est à parcourir.

Ciblé en cours de travail : `make test-fast`. La suite complète `make test` reste
à proposer à l'utilisateur au commit.

## Out of Scope

- `--fold` : conservé tel quel, sujet distinct.
- Toute insensibilité à la casse des mots du langage : elle est conservée.
- Une option de compatibilité ou une période d'avertissement.
- Le format `.fo`, `.sym` ou `.cpr` : ils portent déjà les noms tels qu'écrits.

## Further Notes

- Origine : un `a.asm` de l'utilisateur mêlait `screen_line = 2` et
  `macro SCREEN_LINE()`.
- Un essai préliminaire (annulé) a retiré les majuscules de l'index des macros :
  29 tests du préprocesseur ont échoué, tous parce que le nom d'appel arrive en
  majuscules à la recherche. Cela borne le travail : c'est un point de
  conversion unique à retrouver, pas un changement diffus.
