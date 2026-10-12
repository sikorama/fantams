---
status: accepted
---

# `sin` et `cos` prennent des radians

`sin(x)` et `cos(x)` interprètent `x` en **radians**. Ils l'interprétaient
jusqu'ici en degrés. L'arrondi reste celui de `std::sin` et `std::cos`.

## Contexte

`expr.cpp` calculait `std::sin(a * M_PI / 180.0)`. Le degré est pourtant une unité de saisie
humaine, pas l'unité des fonctions trigonométriques : partout ailleurs — en
mathématiques, dans la libc, dans tous les langages qui exposent un `sin` — le
radian est l'argument, et la conversion depuis les degrés est explicite quand on
en veut.

La conséquence pratique était pire que le simple dépaysement : un `sin` qui
prend des degrés ne se compose pas. Dès qu'un angle vient d'un calcul — une
fraction de tour, un incrément accumulé, une constante `equ` — l'auteur doit
savoir dans quelle unité vit sa variable et se souvenir d'un facteur `180/π`
inversé par rapport à l'habitude.

## Décision

Le radian, sans option ni directive pour revenir aux degrés. Une unité, pas
deux : un mode degrés rendrait la question ouverte sur chaque source,
et la valeur de `sin(45)` dépendrait d'un drapeau invisible dans l'expression —
exactement le genre d'ambiguïté que le projet refuse ailleurs (ADR 0004).

L'écriture en degrés reste possible et se lit dans l'expression :

```
sin(angle*3.14159265/180)
```

## Conséquences

Toute source existante utilisant `SIN`/`COS` produit désormais des octets
**différents**. Le corpus de non-régression comptait 6 sources sur 84
utilisant `SIN`/`COS` : leurs sorties attendues changent et doivent être
re-baselinées sur fantams. Le changement est
silencieux — aucun diagnostic ne la signale, parce qu'aucune expression n'est
en soi suspecte.

Il n'y a pas de constante `pi` dans le langage : l'ajouter est une question
ouverte, distincte de celle-ci.
