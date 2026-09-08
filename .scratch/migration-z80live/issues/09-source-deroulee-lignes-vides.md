# 09: La source déroulée perd les lignes vides et les commentaires

**Status:** needs-triage

**Hors périmètre du chantier « migration ».** Remonté par le mainteneur pendant
la vérification navigateur (ticket 08), sur une source réelle. Déposé ici faute
de chantier d'accueil — à déplacer si un chantier « lisibilité de la source
déroulée » s'ouvre.

## Le constat

    ; un commentaire          |
                              |      org #8000
        org #8000             |  start:
                              |      ld a,1
    start:                    |      ret
        ld a,1                |
                              |
        ret                   |
    ------ entrée ------      |  ------ fantams -E ------

`--beautify` sur la même entrée rend les huit lignes, commentaire et lignes
vides compris.

## Pourquoi c'est une incohérence, et pas seulement un inconfort

`asm_main.cpp` annonce, pour `-E` : « La sortie est MISE EN FORME : la mise en
forme fait partie de la définition de la source déroulée (ADR 0013). »

Et l'ADR 0013 dit, en toutes lettres : « Ce que la règle 4 ne touche pas :
l'alignement des commentaires, qui reste celui de l'auteur, et **les lignes
vides**. »

Les deux ne peuvent pas être vrais en même temps. Le dépouillement ne vient pas
du formateur — il vient du **préprocesseur**, en amont, et la mise en forme
s'applique ensuite à un texte déjà dépouillé.

## Ce qui rend la question non triviale

La source déroulée est « un livrable de premier plan, pas un artefact de
débogage : c'est lui qui rend vérifiable ce que le préprocesseur a compris ».
Deux lectures s'opposent, et il faut trancher laquelle :

- **ce que le préprocesseur a compris** — alors un commentaire n'en fait pas
  partie, et son absence est correcte ; les lignes vides non plus, à la rigueur ;
- **un texte que l'auteur relit pour se convaincre** — alors le dépouillement
  coûte exactement ce que le mainteneur a constaté : « c'est assez brut à
  relire ».

Les deux ne demandent pas le même travail. La seconde suppose de faire survivre
au préprocesseur des choses qu'il n'a aucune raison de garder aujourd'hui, et
elle pose la question des lignes vides **produites par une expansion** — entre
deux tours de boucle, faut-il une ligne vide ? une par tour ?

## Ce qu'il ne faut pas faire à la légère

Changer la sortie de `-E` change des octets :

- `tests/accept_wasm_equiv.sh` compare la source déroulée octet à octet entre
  les deux adaptateurs — le test ne casserait pas, mais son cas de référence
  changerait ;
- z80live affiche cette sortie dans son panneau « Code sent to the assembler »
  et s'en sert pour recaler les numéros de ligne des diagnostics
  (`lineOffset`) : toute ligne ajoutée ou retirée déplace ce recalage.

Le second point est le vrai risque : un diagnostic qui pointe la mauvaise ligne
est pire qu'une source brute à relire.
