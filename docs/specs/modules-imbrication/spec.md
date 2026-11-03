# Modules : imbrication, expansions, nettoyage (chantier)

Source : session du 2026-10-12, échanges avec un utilisateur qui construit des
bibliothèques réutilisables avec les modules. Complète `note.md` (à supprimer une fois le triage fait).

## Décisions acquises

- `MODULE x` **ouvre** un module et `ENDMODULE` (ou `END`) le ferme. Les modules
  s'imbriquent. La forme commutateur (`MODULE x` sans fermeture) disparaît.
- `MODULE OFF` disparaît : `ENDMODULE` (ou `END`) suffit.
- `.` est le **seul** séparateur de noms. Un nom pointé ambigu est une **erreur à
  la définition**, pas une règle de syntaxe particulière.
- Aucune contrainte de spécification ne vient d'un autre outil. Toute mention
  de cette contrainte est retirée (ticket 04), pas seulement niée.
- Un header commun (constantes EQU + gardes `ifndef`) peut être inclus par la
  bibliothèque et par ses utilisateurs. Les `extern` restent dans un fichier
  réservé aux utilisateurs. Le refus « extern et défini dans le même objet » est
  conservé.

## Bugs constatés (reproduits, cause lue dans le code)

- **B1, constante invisible au second `MODULE a`** : le renommage est textuel et
  par bloc (`collectLabels` ne voit que le bloc courant). Correctif : table des
  noms par module, sur tous ses blocs (ticket 01).
- **B2, `@label` dupliqué dans une macro de module** : le renommage de module
  rebaptise `@r` en `a.@r` avant l'expansion, puis `collectLabels(onlyAtPrefixed)`
  ne reconnaît plus le `@`, donc aucun suffixe `__N` (ticket 02).

## Décisions des questions (tranchées le 2026-10-12)

- **Q1** Oui : les `K EQU` plain d'une macro sont scopés par expansion, comme les
  labels. Cela remplace la règle de `pp.cpp:986` (réutilisation = collision),
  à écrire explicitement dans l'ADR du ticket 09. Partager une constante entre
  expansions passe par `@@export` ou par une définition hors de la macro.
- **Q2** Les macros restent globales pour l'instant (appel qualifié `enum.poke(0,0)`
  non supporté). À documenter dans l'ADR « les modules » (ticket 08), à rouvrir
  quand un cas d'usage réel apparaît. Ne bloque rien.
- **Q3** ADR 0025 réécrite, pas supprimée : on garde la décision (« fantams définit
  son langage »), la justification des mots réservés (`BANK`, `SNASET`, …) et le
  rôle du corpus ; on retire la section Contexte chiffrée et toute désignation de
  l'outil tiers (y compris la formule « l'assembleur de référence »). Le numéro
  0025 reste stable.
- **Q4** La comparaison avec un autre assembleur n'a plus lieu d'être. Le corpus ne
  mesure plus qu'une chose : une source compile-t-elle en l'état avec fantams ?
  Si non, c'est le développeur qui corrige la source (pas fantams, et pas
  l'agent, sauf session ouverte au cas par cas). `migration-z80live/` est donc
  reformulée par le ticket 04, pas exclue de son inventaire.
- **Q5** Dans `migration-z80live/`, on réécrit la spec et les tickets non résolus.
  Les tickets résolus (`[x]`) restent un historique daté et sont exclus du grep
  de vérification du ticket 04 ; s'il en remonte, le développeur décide.

## Décisions sur l'expansion (tranchées le 2026-10-12, Q6 à Q10)

- **Q6** Dans une expansion (macro, `repeat`, `while`, `for`), un label **écrit en dur**
  est local à l'expansion, sans `@`. Un label dont le nom **contient une
  substitution `{…}`** (`{name}:`, `mon_label_{name}:`, `{name}_label:`) garde le nom
  construit : c'est l'appelant qui l'a nommé. Deux appels qui construisent le même
  nom sont une erreur (`duplicate symbol`), voulue.
- **Q7** `@` devient le marqueur d'un label **global à usage unique** : `@entry:`
  définit `entry`, tel quel, et la macro ne peut être développée qu'une fois. Un
  second appel donne `duplicate symbol`, enrichi pour dire que la macro porte un
  label `@`. `@@export` disparaît (erreur nommée : « use `@` on the label »). Même
  règle pour `STRUCT type @x` et pour les constantes. Cela **inverse** le sens
  actuel de `@` (« unique par expansion ») : sûr, car une macro développée plusieurs
  fois avec l'ancien `@` casse tout de suite sur un `duplicate symbol`.
- **Q7b** Le `@` n'est pas dans le nom : `@entry:` définit `entry`.
- **Q7c** `@x` hors d'une expansion (niveau supérieur, module) est une erreur nommée.
- **Q8** Sans objet : `@` garde un rôle, il n'est pas supprimé.
- **Q9** Le préfixe d'expansion est `<macro>__<n>.` (`poke__1.retry`) ; un `repeat`,
  `while`, `for` prend son mot-clé (`repeat__3.x`) ; les expansions imbriquées se
  composent comme les modules ; le motif `__<chiffres>` est réservé aux noms écrits
  par l'utilisateur ; `--sym` montre ces noms. Le numéro est un compteur global.
- **Q10** Un nom local à l'expansion masque le même nom global ; un nom que
  l'expansion ne définit pas reste visible tel quel (même règle que les modules
  imbriqués, ADR 0037).

Écartée : un `{}` vide dans le nom pour marquer un label global (peu élégant, et
effets indésirables si un paramètre passé est vide).

## Tickets

| # | Titre | Blocked by | Type |
|---|---|---|---|
| 01 | Constante visible dans un module rouvert (B1) | — | bug |
| 02 | `@label` dans une macro de module, expansions multiples (B2) | — | bug |
| 03 | Doc : `@@export`, `extern` qualifié, pattern header | — | doc |
| 04 | Retirer toute mention de l'autre assembleur comme contrainte | — | nettoyage |
| 05 | Décision : modules imbriqués, ENDMODULE ferme, MODULE OFF retiré | — | ADR |
| 06 | Implémenter l'imbrication (pile, préfixe en chaîne) | 01, 05 | feature |
| 07 | Ambiguïté des noms pointés : erreur à la définition | 06 | feature |
| 08 | ADR « les modules » (il n'en existe aucun) | 05, 07 | ADR |
| 09 | ADR « expansion = espace de noms », séparateur, `__N` réservé | — | ADR |
| 10 | Implémenter le scope par expansion (remplace le suffixe `@`) | 09 | feature |

Chaque ticket : un fichier `issues/NN-<slug>.md`, selon `docs/agents/issue-tracker.md`.

## Où les changements toucheront (lecture du code, pas de modification)

- Modules : `pp.cpp:1554-1584` (handler MODULE), `pp.cpp:989-1046` (collectLabels),
  `pp.cpp:1103` (MODULE absent de la table des blocs, à reconsidérer),
  `pp.cpp:1322` (mention d'autres assembleurs dans un commentaire).
- Tests : `tests/pp_test.cpp:525-536` (MODULE OFF et forme commutateur).
- Exemples : `examples/struct_module.asm` (seul exemple avec MODULE).
- Doc : `docs/syntax.md:898-906` (@@export, exemple faux), `:918`, `:967-974`
  (modules), `:1066` (mention d'autres assembleurs), `CONTEXT.md:135` et `:144-145`.
- Mentions d'un autre outil : `CONTEXT.md`, `pp.cpp`, `docs/adr/` (0001, 0002,
  0005, 0007, 0008, 0010, 0020, 0025, 0029), `tests/*`, `.scratch/*` (voir ticket 04).
