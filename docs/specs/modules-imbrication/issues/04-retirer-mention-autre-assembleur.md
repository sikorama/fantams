Status: resolved
Blocked by: —

# 04: Retirer toute mention de l'autre assembleur comme contrainte de spécification

Règle : aucune référence à un autre assembleur ne sert d'argument pour bloquer
une évolution. Il ne suffit pas d'écrire « ce n'est plus une contrainte » : la
mention disparaît.

**Inventaire** (à refaire au moment du ticket : chercher les formules qui désignent
un autre outil, et les noms d'outils eux-mêmes, dans tout le dépôt hors `build*/`
et `.git/`) :
- `pp.cpp:1103`, `pp.cpp:1322-1323`, `pp.cpp:1555-1556` (commentaires) ;
- `docs/syntax.md:1066` (liste d'outils) ;
- `CONTEXT.md` (glossaire, définition du module par contraste) ;
- `docs/adr/0001, 0002, 0005, 0007, 0008, 0010, 0020, 0025, 0029` ;
- `tests/*.cpp` (commentaires) ;
- `.scratch/` : `placement-en-source/spec.md`, `modules-imbrication/note.md`,
  `migration-z80live/` (spec et tickets non résolus seulement).

**Ne pas toucher** : « référence » au sens courant (« référence à un symbole »,
« référence croisée »). Le grep se fait sur la formule complète, pas sur le mot.

**ADR 0025** (Q3, tranchée) : la réécrire, pas la supprimer. Garder la décision
(« fantams définit son langage »), la justification des mots réservés et le
rôle du corpus ; retirer la section Contexte chiffrée et toute désignation de
l'outil tiers, « l'assembleur de référence » compris (formule à retirer aussi
des ADR qui l'emploient).

**Corpus et `migration-z80live/`** (Q4, Q5, tranchées) : la comparaison avec un
autre assembleur disparaît. Le corpus ne mesure plus que « la source compile-t-elle
en l'état avec fantams ? » ; si non, le développeur corrige la source, pas
l'agent. Reformuler en ce sens la spec et les tickets **non résolus** de
`.scratch/migration-z80live/`. Les tickets résolus (`[x]`) restent tels quels.

**Critère** : `grep -ri` sur les formules ci-dessus ne rend plus rien hors
historique git et hors Q4. Le `CHANGELOG`/ADR ne doit plus citer l'outil.

## Réponse

Mentions retirées (environ 120 lignes, une cinquantaine de fichiers) : commentaires
de `pp.cpp`, `pp.h`, `asm.cpp`, `z80.cpp`, `expr.cpp`, `parser.cpp`, `keywords.*`,
`sna.*`, `sym.h`, `examples/aliased_org.asm` ; commentaires et descriptions des
tests (`pp_test`, `asm_test`, `expr_test`) ; ADR 0001, 0003, 0005, 0006, 0007, 0008,
0010, 0011, 0016, 0019, 0020, 0021, 0030 ; ADR 0025 réécrite (titre « fantams
définit son langage, et le corpus ne mesure que ce qui compile en l'état ») ;
`CONTEXT.md` (entrée Corpus, Texte distinct, Cas de référence, Module) ;
`docs/syntax.md` (phrase d'`incbin`) ; `docs/spec-chaine-outils.md` ; spec et
tickets ouverts de `migration-z80live/` (« comparaison » → « mesure sur le corpus »).
Code exécutable inchangé ; `make test` vert (14 suites).

Restent, volontairement : le ticket 02 de `migration-z80live/` (résolu, historique),
`docs/recherche/` (« compatibility » du matériel et de SDCC), et les fichiers de ce
chantier qui décrivent le nettoyage lui-même. `note.md` est à supprimer (la spec le dit).

**À relire par le développeur** : les ADR réécrits en profondeur, en particulier
0010 (chaîne décalée), 0020 (catalogue : retrait de la clause « octets identiques
sur 89 formes », et du paragraphe « dix gestionnaires », qui décrivait l'autre
outil) et 0021 (radians). Les raisons sont gardées, mais les phrases sont
reformulées sans pouvoir être revérifiées contre l'ancien outil.

**Décisions laissées au développeur** : l'expression « plafond de compatibilité »
(ADR 0011, 0015, `docs/principes.md`) et le terme « engagement de compatibilité »
(§12.1 de `spec-chaine-outils.md` et `spec-etage-c1.md`) désignent le corpus ou la
sortie attendue, pas un outil nommé : laissés tels quels.
