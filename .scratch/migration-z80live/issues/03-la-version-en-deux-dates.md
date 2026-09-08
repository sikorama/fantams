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

**Status:** ready-for-agent

- [ ] Une option de l'adaptateur CLI rend les deux dates, accolées, sur une seule
      ligne
- [ ] Le calcul n'exige ni git, ni réseau, ni option de compilation particulière :
      il fonctionne à l'identique dans l'environnement de construction natif,
      dans le conteneur de compilation WASM, et sur une archive extraite sans
      dépôt
- [ ] La date de version vit dans les sources et son incrémentation est un geste
      d'une seule ligne
- [ ] La même option rend la même forme à travers l'adaptateur WASM
- [ ] Aucun ordre entre versions n'est défini et rien n'analyse la chaîne : elle
      est faite pour être lue
