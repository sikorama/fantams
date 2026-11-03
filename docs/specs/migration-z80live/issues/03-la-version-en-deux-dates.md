# 03: La version en deux dates

**What to build:** fantams sait dire qui il est. Une option de l'adaptateur CLI
rend deux dates :

- une **date de version portée dans l'arbre des sources**, que le mainteneur
  incrémente quand il le décide — elle dit *ce qui a été voulu comme livraison* ;
- une **date de compilation**, obtenue par le macro standard du préprocesseur —
  elle dit *quand cet artefact-là a été produit*.

C'est l'écart entre les deux qui répond à la question « cet artefact est-il à
jour ». Aucun identifiant de commit : il dépend du dépôt, et le dépôt pourra
être recréé ou son historique aplati.

L'option étant sur la surface `argv`, elle est atteignable par l'adaptateur WASM
sans rien ajouter à celui-ci.

**Blocked by:** 01 (le manifeste de sources unique) — la vérification à travers
l'adaptateur WASM exige un artefact WASM qui édite ses liens.

**Status:** resolved

- [x] Une option de l'adaptateur CLI rend les deux dates, accolées, sur une seule
      ligne
- [x] Le calcul n'exige ni git, ni réseau, ni option de compilation particulière :
      il fonctionne à l'identique dans l'environnement de construction natif,
      dans le conteneur de compilation WASM, et sur une archive extraite sans
      dépôt
- [x] La date de version vit dans les sources et son incrémentation est un geste
      d'une seule ligne
- [x] La même option rend la même forme à travers l'adaptateur WASM
- [x] Aucun ordre entre versions n'est défini et rien n'analyse la chaîne : elle
      est faite pour être lue

## Commentaires

Résolu. `version.h` porte les deux dates et la ligne qui les accole ;
`--version` la rend sur la sortie standard et sort, avant toute exigence de
fichier — un artefact qui ne saurait plus assembler doit encore savoir dire son
âge.

    $ ./fantams --version
    fantams 2026-09-08 (compile 2026-09-08)

La date de version est une ligne de `version.h` : l'incrémenter, c'est éditer
cette ligne-là et rien d'autre. La date de compilation vient de `__DATE__`,
rendue en AAAA-MM-JJ pour que les deux se lisent dans la même graphie et que
l'écart saute aux yeux — ni git, ni réseau, ni option de compilation, donc la
même valeur dans la distrobox, dans le conteneur emsdk et sur une archive
extraite sans dépôt.

`version.h` est un en-tête sans module : deux constantes et trois lignes de
code ne valaient pas une entrée au manifeste ni une bibliothèque CMake de plus.

L'option étant sur la surface `argv`, l'adaptateur WASM la sert sans rien
ajouter : c'est vérifié par `tests/accept_wasm_equiv.sh` (ticket 04), qui
compare la forme et la date de version des deux côtés — et non la date de
compilation, dont l'écart est justement ce qu'on veut pouvoir constater.
