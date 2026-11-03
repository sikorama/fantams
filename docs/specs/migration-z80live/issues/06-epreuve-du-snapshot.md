# 06: L'épreuve du snapshot

**What to build:** la première **épreuve** du projet — l'exécution d'un artefact
sur une machine pour constater qu'elle l'accepte et ce qu'elle en fait. Un cas de
référence est assemblé, le snapshot produit est posté dans l'émulateur par son
API locale, et l'exécution est constatée.

Son autorité porte sur la **recevabilité** de l'artefact, jamais sur ses octets :
la règle établie — un octet se teste depuis une image fabriquée à la main, sans
machine — reste intacte. C'est un instrument de mesure, de la famille du corpus
et de la machine réelle.

Le cas de référence est **écrit**, jamais extrait du corpus (ADR 0029).

Le cycle de vie reprend celui du test de fumée du frontal sans affichage de
l'émulateur, invariant pour invariant. Le relevé de l'API est versionné dans les
recherches du dépôt et sert de référence ; le contrat est par ailleurs servi par
l'émulateur lui-même.

**Blocked by:** 01 (le manifeste de sources unique), 04 (l'équivalence natif ≡
WASM) — l'arête vers 04 est une réutilisation de sa discipline de saut, pas une
dépendance de fond.

**Status:** resolved

- [x] Un cas de référence dédié, écrit pour cette épreuve, minimal et autonome
- [x] L'émulateur est démarré avec une **configuration jetable** : l'épreuve ne
      dépend ni ne touche à la configuration de travail du mainteneur
- [x] L'attente du démarrage se fait par **sondage**, pas par une attente fixe
- [x] L'épreuve constate que l'émulation **avance réellement** avant de conclure :
      un émulateur figé ne passe pas pour un succès
- [x] Le snapshot est chargé par l'endpoint unique qui accepte tous les
      conteneurs, le format étant autodétecté par ses octets magiques
- [x] L'effet de l'exécution est constaté par une lecture d'état, et la valeur
      attendue est celle que le cas de référence a été écrit pour produire
- [x] L'arrêt passe par la voie ordonnée, et le code de sortie du processus est
      vérifié
- [x] L'épreuve se saute — et non échoue — quand le frontal sans affichage de
      l'émulateur est absent : construire fantams n'impose pas un second dépôt
- [x] L'épreuve n'affirme rien sur les octets produits

## Commentaires

Résolu. `tests/epreuve_snapshot.sh`, avec `tests/epreuve/marqueur.asm` pour cas
de référence — **écrit** pour cette épreuve, jamais extrait du corpus.

Le cas de référence pose une valeur connue à une adresse connue puis boucle :

    MARQUEUR equ 0x9000
    start:  di / ld sp,0xbf00 / ld a,0x5a / ld (MARQUEUR),a
            ld hl,0xcafe / ld (MARQUEUR+1),hl
    boucle: jr boucle

Dans les octets du snapshot, ces trois octets valent **zéro** — l'épreuve le
constate avant de démarrer la machine, plutôt que de l'affirmer. Les relire à
`5a fe ca` prouve donc une **exécution**, et pas seulement un chargement : c'est
le contrôle négatif intégré au cas.

Le cycle de vie reprend celui du test de fumée du frontal sans affichage,
invariant pour invariant : configuration jetable (`-C` sur un `mktemp -d`),
attente du démarrage par **sondage** de `/api/ping`, vérification que le
compteur de trames **avance** avant comme après le chargement, arrêt par
`POST /api/quit` avec contrôle du code de sortie. Le chargement passe par
`POST /api/media`, l'endpoint unique qui accepte tous les conteneurs, le format
étant autodétecté par ses octets magiques. L'effet est constaté par
`GET /api/ram?...&view=cpu` — ce que le Z80 voit réellement, donc ce que le
programme a écrit, et non une banque choisie par le harnais.

**Aucun point d'arrêt n'est posé, et c'est délibéré.** Tant que l'épreuve n'en a
pas besoin, elle n'écrit pas de traducteur entre notre notation physique et
celle de l'émulateur — graphie identique, sémantique différente. Le jour où
elle en posera un, la traduction vivra à un seul endroit, nommée pour ce
qu'elle est. Le faux ami est relevé dans `docs/recherche/amspirit-pilotage-api.md`
§5 ; ne pas le réveiller sans besoin est la meilleure façon de ne pas le
réinventer faussement.

**Une garde de plus que le patron.** Si un serveur répond déjà sur le port,
l'épreuve se **saute** au lieu de continuer : c'est l'instance de quelqu'un, on
ne pilote pas une machine qu'on n'a pas démarrée — et surtout on ne lui envoie
pas `/api/quit`.

### Vérifiée pour de bon

    epreuve : la machine a accepte le snapshot et l'a execute — 0x9000 = 5afeca
    epreuve : arret ordonne, code de sortie 0 (9 -> 19 trames)

Le frontal sans affichage n'était pas construit : il l'a été dans un répertoire
de construction séparé (`meson setup -Dbuild_headless_frontend=true`, cœur
pré-construit), sans toucher à celui du dépôt de l'émulateur. `ctest` complet,
toutes dépendances présentes : **17/17**. Sans elles, `epreuve_snapshot` et
`accept_wasm_equiv` sont rapportés `Skipped`.

L'épreuve n'affirme rien sur les octets produits : les octets se testent depuis
une image fabriquée à la main, sans machine, et cette règle reste entière.
