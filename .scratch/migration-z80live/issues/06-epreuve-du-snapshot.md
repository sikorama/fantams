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

**Status:** ready-for-agent

- [ ] Un cas de référence dédié, écrit pour cette épreuve, minimal et autonome
- [ ] L'émulateur est démarré avec une **configuration jetable** : l'épreuve ne
      dépend ni ne touche à la configuration de travail du mainteneur
- [ ] L'attente du démarrage se fait par **sondage**, pas par une attente fixe
- [ ] L'épreuve constate que l'émulation **avance réellement** avant de conclure :
      un émulateur figé ne passe pas pour un succès
- [ ] Le snapshot est chargé par l'endpoint unique qui accepte tous les
      conteneurs, le format étant autodétecté par ses octets magiques
- [ ] L'effet de l'exécution est constaté par une lecture d'état, et la valeur
      attendue est celle que le cas de référence a été écrit pour produire
- [ ] L'arrêt passe par la voie ordonnée, et le code de sortie du processus est
      vérifié
- [ ] L'épreuve se saute — et non échoue — quand le frontal sans affichage de
      l'émulateur est absent : construire fantams n'impose pas un second dépôt
- [ ] L'épreuve n'affirme rien sur les octets produits
