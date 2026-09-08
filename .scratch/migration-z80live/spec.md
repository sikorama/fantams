# Spec — La migration : remettre z80live en marche sur fantams

Status: ready-for-agent

## Problem Statement

z80live ne peut plus être reconstruit. Ses trois instruments — l'assemblage dans
le navigateur, la comparaison sur le corpus, et l'épreuve sur émulateur qu'on
veut ajouter — sont tous les trois hors service, et pour la même cause.

Le script de construction WASM déclare sa propre liste de fichiers sources, et
cette liste a cessé d'être complète : quatre modules ajoutés pendant les étages
A, B et C1 n'y figurent pas, alors que l'adaptateur CLI les appelle. L'édition
de liens échoue sur symboles indéfinis. L'artefact WASM effectivement livré à
z80live précède les trois étages : il a été compilé avant que l'étage A ne
commence.

Le dépôt avait déjà nommé cette faute, un cran plus bas : un commentaire du
fichier de configuration CMake constate qu'un test manquait à sa liste alors que
le Makefile le construisait, et conclut que « deux listes, dont une seule
complète, est le genre d'écart qui se paie en CI ». La même faute s'est reproduite
sur les **sources** au lieu des **tests**, et personne ne l'a vue pendant trois
étages, parce que rien ne la teste.

Rien n'est cassé dans l'interface, en revanche : z80live n'emploie que quatre
options de l'adaptateur CLI, et les quatre s'analysent aujourd'hui à l'identique,
avec la même sémantique. La migration n'est donc pas un rattrapage d'interface :
c'est une panne de construction, plus l'absence de tout verrou qui l'aurait
signalée.

S'y ajoute une aggravation : fantams ne sait pas dire qui il est. Aucune option
ne rend sa version, aucun identifiant n'est embarqué dans l'artefact. Quand une
vérification échouera, rien ne permettra de distinguer « fantams a un bug » de
« cet artefact a trois étages d'âge » — ce qui est précisément la situation
présente, et ce qui a permis à la péremption de durer.

Enfin, le pointeur de sous-module que z80live enregistre pour fantams est
vingt-sept commits en arrière, et son avancement n'a jamais été commité : le
dépôt consommateur affirme une version qu'il n'utilise pas.

## Solution

Une réparation **et** un verrou. La réparation seule remettrait z80live en
marche ; le verrou est ce qui empêche la panne de se reproduire silencieusement,
et c'est lui qui justifie l'essentiel du travail.

Du point de vue du mainteneur, après ce chantier :

- z80live assemble à nouveau, avec un fantams à jour, et **affiche quelle version
  de fantams il fait tourner** ;
- la liste des fichiers sources n'existe qu'à **un seul endroit**, de sorte que
  la dérive qui a causé la panne devienne structurellement impossible plutôt que
  détectable ;
- l'adaptateur WASM est **testé comme n'importe quelle autre couture** : le même
  `argv` à travers l'adaptateur natif et l'adaptateur WASM doit rendre les mêmes
  octets, ce qui est exactement la propriété qu'un artefact périmé viole ;
- la comparaison sur le corpus retourne en état de marche ;
- une **épreuve** existe : l'artefact produit part dans l'émulateur et la machine
  l'exécute — la première vérification du projet qui porte sur la *recevabilité*
  d'un artefact et non sur ses octets ;
- le dépôt consommateur épingle fantams délibérément, par un commit qui le dit.

## User Stories

1. En tant que mainteneur, je veux pouvoir reconstruire l'artefact WASM sans
   échec d'édition de liens, afin que z80live redevienne utilisable.
2. En tant que mainteneur, je veux que la liste des fichiers sources du cœur soit
   déclarée une seule fois, afin qu'aucun point d'entrée de construction ne puisse
   en avoir une version incomplète.
3. En tant que mainteneur, je veux que l'ajout d'un module au cœur n'exige aucune
   édition dans les scripts de construction, afin que la prochaine étape ne
   reproduise pas la panne.
4. En tant que mainteneur, je veux qu'une option rende la version de fantams,
   afin de pouvoir distinguer un bug d'une péremption d'artefact.
5. En tant que mainteneur, je veux que cette version soit une **date** et non un
   identifiant de commit, afin qu'elle survive à un nettoyage d'historique ou à
   un dépôt recréé.
6. En tant que mainteneur, je veux que la version distingue *ce que j'ai voulu
   livrer* de *quand cet artefact a été compilé*, afin que l'écart entre les deux
   réponde à lui seul à la question « cet artefact est-il à jour ? ».
7. En tant que mainteneur, je veux que le calcul de la version n'exige ni git ni
   réseau, afin qu'elle fonctionne identiquement dans la distrobox, dans le
   conteneur de compilation WASM, et sur une archive extraite sans dépôt.
8. En tant qu'utilisateur de z80live, je veux voir affichée la version de fantams
   qui tourne, afin de savoir si le comportement que j'observe vient d'une
   version ancienne.
9. En tant qu'utilisateur de z80live, je veux que cette version s'affiche sans
   que rien ne l'analyse, afin qu'un changement de sa forme ne casse rien.
10. En tant que mainteneur, je veux un test qui compare octet à octet la sortie
    de l'adaptateur natif et celle de l'adaptateur WASM sur la même entrée, afin
    qu'un artefact périmé fasse rougir la suite de tests.
11. En tant que mainteneur, je veux que ce test se **saute** proprement quand
    l'artefact WASM ou node est absent, afin que `ctest` ne rougisse pas pour une
    dépendance manquante.
12. En tant que mainteneur, je veux que le test de l'adaptateur WASM vive dans
    fantams et non dans le dépôt consommateur, afin que ce soit fantams qui
    garantisse la justesse de son propre adaptateur.
13. En tant que mainteneur, je veux que le binaire natif soit constructible dans
    le répertoire du sous-module, afin que la comparaison sur le corpus puisse
    tourner.
14. En tant que mainteneur, je veux que le dépôt consommateur enregistre par un
    commit explicite la version de fantams qu'il utilise, afin que le pointeur
    cesse d'affirmer une version qui n'est pas celle en place.
15. En tant que mainteneur, je veux une liste de contrôle écrite pour la
    vérification dans le navigateur, afin que la seule étape qu'aucun test ne peut
    exécuter ne puisse pas être déclarée faite sans l'avoir été.
16. En tant que mainteneur, je veux une épreuve qui poste un artefact dans
    l'émulateur et constate que la machine l'accepte, afin de vérifier ce qu'aucun
    test unitaire ne peut poser.
17. En tant que mainteneur, je veux que cette épreuve porte sur le snapshot et
    sur rien d'autre pour l'instant, afin qu'elle soit disponible avant tout
    travail sur les conteneurs plutôt qu'après.
18. En tant que mainteneur, je veux que l'épreuve démarre l'émulateur avec une
    configuration jetable, afin qu'elle ne dépende ni ne touche à ma
    configuration de travail.
19. En tant que mainteneur, je veux que l'épreuve attende le démarrage par
    **sondage** et non par une attente fixe, afin qu'elle soit rapide quand tout
    va bien et fiable quand la machine est chargée.
20. En tant que mainteneur, je veux que l'épreuve constate que l'émulation
    **avance réellement** avant de conclure, afin qu'un émulateur figé ne passe
    pas pour un succès.
21. En tant que mainteneur, je veux que l'épreuve arrête l'émulateur par la voie
    ordonnée et vérifie son code de sortie, afin qu'elle ne laisse pas de
    processus derrière elle.
22. En tant que mainteneur, je veux que l'épreuve se **saute** quand le binaire
    de l'émulateur est absent, afin qu'elle n'impose pas un second dépôt à
    quiconque construit fantams.
23. En tant que mainteneur, je veux que l'épreuve n'ait aucune autorité sur les
    octets, afin que la règle établie — un octet se teste depuis une image
    fabriquée à la main, sans machine — reste intacte.
24. En tant qu'agent travaillant dans ce dépôt, je veux que la traduction entre
    la notation d'adresse physique du projet et celle de l'émulateur vive à un
    seul endroit nommé, afin de ne pas la réinventer faussement — les deux
    graphies sont identiques et leurs sémantiques diffèrent.
25. En tant que mainteneur, je veux qu'aucune documentation produite pendant ce
    chantier ne cite un identifiant de commit, afin qu'un nettoyage d'historique
    ne rende pas ces documents faux.
26. En tant que mainteneur, je veux que rien de ce chantier n'ajoute un format de
    sortie à l'aiguillage par extension de l'adaptateur CLI, afin de ne pas
    amorcer la dérive que l'ADR 0007 décrit.
27. En tant que contributeur, je veux que l'échec de construction WASM soit
    bruyant et immédiat plutôt que silencieux, afin de ne pas découvrir la panne
    trois étages plus tard.
28. En tant que mainteneur, je veux savoir, à la fin de ce chantier, que les deux
    critères de clôture sont tenus séparément : *z80live remarche*, et *la panne
    ne peut plus se reproduire silencieusement*.

## Implementation Decisions

### La liste de sources canonique

Le cœur est déclaré **une seule fois**, dans un manifeste que les trois points
d'entrée de construction — le Makefile, la configuration CMake et le script de
construction WASM — **lisent** au lieu de le redéclarer.

Deux alternatives ont été examinées et écartées. Globber les fichiers ne
convient pas : le générateur CMake ne re-globe pas à l'apparition d'un fichier,
donc la dérive reviendrait par la CI. Faire extraire la liste du Makefile par le
script WASM ne convient pas non plus : cela remplace une dérive par un analyseur
de Makefile, c'est-à-dire une fragilité.

Le manifeste ne contient que les modules du cœur. Les deux outils portant un
`main` en sont exclus, et chaque point d'entrée ajoute le sien.

### La version

Deux dates, jamais un identifiant de commit — un identifiant de commit dépend du
dépôt, et le dépôt pourra être recréé ou son historique aplati.

- Une **date de version portée dans l'arbre des sources**, que le mainteneur
  incrémente quand il le décide. Elle dit *ce qui a été voulu comme livraison*.
- Une **date de compilation**, obtenue par le macro standard du préprocesseur.
  Elle dit *quand cet artefact-là a été produit*, et n'exige aucune plomberie :
  ni git, ni option de compilation, ni réseau.

La forme rendue accole les deux. C'est l'écart entre elles qui répond à la
question « cet artefact est-il à jour ». La version est exposée par une option de
l'adaptateur CLI, donc atteignable par l'adaptateur WASM sans rien ajouter, et
z80live l'**affiche telle quelle** — rien ne l'analyse, aucun ordre entre
versions n'est défini.

### L'adaptateur WASM est un adaptateur

Au sens de l'ADR 0001 : une couche fine qui relie un hôte au cœur. Il en découle
qu'il se teste, et qu'il n'a droit à aucune règle du langage qui lui soit propre.
Le contrat qu'il expose est exactement celui de l'adaptateur CLI — un `argv` et
un système de fichiers, aucune fonction du cœur exportée — ce qui rend
l'équivalence entre les deux adaptateurs vérifiable sans rien construire de neuf.

La construction reste déclenchée par le dépôt consommateur, qui appelle le script
de fantams ; fantams est responsable de la **justesse** de ce script, pas du
moment où il tourne. Les artefacts de construction ne sont pas versionnés dans
fantams : ce sont des produits, et le dépôt exclut déjà le binaire natif pour la
même raison.

### Le lien entre les deux dépôts

Le dépôt consommateur **épingle** fantams délibérément, en consommateur ordinaire,
et fait avancer son pointeur par un commit qui l'énonce. Le régime alternatif —
suivre la branche principale en continu — a été écarté : dès qu'une épreuve
existe, « quelle version de fantams a produit cet artefact » devient la question
qui décide si un échec est un bug ou une dérive de version.

Le binaire natif doit par ailleurs être constructible dans le répertoire du
sous-module, faute de quoi la comparaison sur le corpus ne peut pas tourner. Cette
panne est **distincte** de celle du WASM et se répare séparément.

### L'épreuve

Un instrument de mesure, de la famille du corpus et de la machine réelle, et non
un banc de test des octets. Son autorité porte sur la **recevabilité** d'un
artefact : la machine l'accepte-t-elle, et qu'en fait-elle. Les tests unitaires
gardent toute autorité sur les octets, depuis une image fabriquée à la main, ce
que la doctrine des coutures énonce déjà.

Elle pilote l'émulateur par son API HTTP locale, dont le contrat est servi par
l'émulateur lui-même et vérifié chez lui par un test dédié. Le relevé de cette
API est versionné dans les recherches du dépôt.

Son cycle de vie reprend celui du test de fumée du frontal sans affichage de
l'émulateur, invariant pour invariant : configuration jetable, attente du
démarrage par sondage, vérification que le compteur de trames avance, arrêt par
la voie ordonnée avec contrôle du code de sortie.

Le chargement d'un artefact passe par l'endpoint unique qui accepte tous les
conteneurs, le format étant autodétecté par ses octets magiques. Pour ce chantier,
un seul artefact est éprouvé : le snapshot.

### Le faux ami des notations d'adresse physique

L'émulateur accepte une notation physique de **graphie identique** à celle de
l'ADR 0005 et de **sémantique différente** : numéro de banque en hexadécimal
contre décimal, et surtout un décalage qui **reporte** au-delà de la fenêtre de
16 K contre une adresse logique **masquée**. Les deux notations désignent donc
des octets différents. Le numérotage des banques, lui, concorde.

Si l'épreuve pose un point d'arrêt, la traduction vit à un seul endroit, nommée
pour ce qu'elle est. Elle est mécanique — banque en hexadécimal, décalage par le
masque que l'ADR définit déjà — mais elle ne doit pas être réinventée à chaque
appel.

### Ce que ce chantier n'ajoute pas

Aucun format de sortie n'est ajouté à l'aiguillage par extension de l'adaptateur
CLI. C'est la dérive que l'ADR 0007 décrit — chaque format finissant par avoir
ses propres options de découpage, mutuellement incohérentes — et le report des
conteneurs n'a de sens que si l'attente est tenue proprement.

### Documentation

Aucun document produit ne cite d'identifiant de commit : un nettoyage
d'historique les détruira. On cite des noms d'étage, de fichier et de branche.

## Testing Decisions

### Ce qui fait un bon test ici

Un test porte sur le **comportement externe** à la couture, jamais sur un détail
d'implémentation. À cette couture, le comportement externe est : un `argv`, des
fichiers en entrée, des fichiers et des diagnostics en sortie. Un test qui
inspecterait une structure interne du cœur pour vérifier une propriété de
construction serait au mauvais niveau.

### La couture : une seule, existante

La surface `argv` + fichiers de l'adaptateur CLI. C'est la plus haute disponible,
et elle suffit à tout ce chantier, parce que l'adaptateur WASM n'exporte aucune
fonction du cœur : son contrat *est* le contrat CLI. Les trois coutures internes
du projet — assembler, lier, empaqueter — ne sont pas touchées.

Aucune couture nouvelle n'est créée.

### Les tests

- **Équivalence des adaptateurs.** Le même `argv` et les mêmes fichiers, à
  travers l'adaptateur natif et l'adaptateur WASM, rendent les mêmes octets.
  C'est le critère qui remplace celui en place, lequel se contentait de vérifier
  la signature du snapshot et un drapeau de succès — et qu'un artefact vieux de
  trois étages satisfait sans difficulté.
- **La version.** L'option rend les deux dates dans la forme attendue, à travers
  les deux adaptateurs.
- **L'épreuve du snapshot.** Un cas de référence est assemblé, l'artefact est
  posté dans l'émulateur, l'exécution est constatée. Elle n'affirme rien sur les
  octets.
- **La liste de sources** n'a pas de test : avec un manifeste unique lu par les
  trois points d'entrée, la propriété est structurelle. Ce qui reste vérifiable
  — que l'artefact construit fonctionne — l'est par l'équivalence.

### Discipline de saut

Les tests dont la dépendance est absente se **sautent**, par le mécanisme prévu
à cet effet par le lanceur de tests, au lieu d'échouer. Trois dépendances sont
concernées : l'artefact WASM, l'exécutable node, et le frontal sans affichage de
l'émulateur. Sans cela, la suite rougit pour de mauvaises raisons dans
l'environnement où elle tourne — l'outillage de construction CMake et le
conteneur de compilation WASM ne vivent pas au même endroit.

Un saut doit être bruyant dans le rapport : un test sauté n'est pas un test
réussi, et l'ensemble du verrou repose sur cette distinction.

### Prior art

Les quatre tests d'acceptation existants sont le moule exact : un script shell
sous le répertoire des tests, câblé au lanceur avec le répertoire de travail à la
racine et le chemin du binaire construit passé par l'environnement. Les nouveaux
tests le suivent sans rien inventer.

Pour le cycle de vie de l'épreuve, le prior art est extérieur : le test de fumée
du frontal sans affichage de l'émulateur, dont les invariants sont énumérés dans
les décisions d'implémentation.

## Out of Scope

- **Les conteneurs** — en-tête AMSDOS, CPR, DSK, CRO — et la couture
  d'empaquetage qui les porte. Reportés après ce chantier, et à ce moment-là
  construits comme une couture et non comme des cas particuliers.
- **La mesure de couverture et le choix des cas de référence.** Le classement des
  constructions par diversité syntaxique, la séparation entre ce que fantams ne
  sait pas faire et ce qui demande un portage, la proposition de sources au
  mainteneur et son déverrouillage nommé : chantier suivant.
- **L'anonymisation du rapport de comparaison** — identifiant opaque au lieu du
  nom, compte par catégorie d'erreur au lieu du texte des diagnostics. Va avec la
  mesure.
- **Les profils supplémentaires** pour z80live : CPC+, ZX. Explicitement après.
- **L'exploitation dans z80live des capacités de l'étage C1** : scripts de lien,
  profils, table des symboles, fichiers objets, assemblage séparé. Le dépôt
  consommateur n'en utilise aucune, et ce chantier ne change pas cela.
- **L'étage C2** — le linker qui vérifie — et la compression, qui n'a pas d'étage
  d'accueil.
- **L'installation d'un environnement de compilation WASM natif** dans la
  distrobox, qui rendrait toute la chaîne testable d'un seul endroit. Souhaitable,
  hors du chemin critique.
- **La réécriture de l'historique publié.** Décidée non faite : la fuite est
  faite, et une réécriture ne reprend pas ce qui est indexé. Un nettoyage viendra
  pour d'autres raisons, à la fin des chantiers.
- **Tout travail d'interface** dans z80live au-delà de l'affichage de la version.

## Further Notes

**Les deux critères de clôture sont distincts**, et c'est le second qui justifie
la majeure partie du travail. « z80live remarche » serait atteint en ajoutant les
quatre fichiers manquants au script de construction : dix minutes. « La panne ne
peut plus se reproduire silencieusement » demande le manifeste unique, la
version, et l'équivalence des adaptateurs. Un ticket qui livrerait le premier
sans le second aurait raté la spec.

**Rien n'est cassé dans l'interface.** Il est important qu'aucun ticket ne parte
chercher une incompatibilité : les quatre options qu'emploie le dépôt
consommateur s'analysent aujourd'hui à l'identique et avec la même sémantique.
La seule chose que l'étage C1 a changée pour z80live, c'est qu'il existe
désormais des capacités qu'il n'utilise pas.

**La faute a un précédent dans ce dépôt**, un cran plus bas, et il est consigné
en commentaire dans la configuration CMake : une liste de tests incomplète face à
une liste complète. La conclusion qui y est écrite — deux listes dont une seule
complète se paient en CI — est la justification du manifeste unique, et elle
vient du dépôt lui-même.

**Une contrainte de publication court en arrière-plan de tout ce chantier.**
L'intégralité du corpus n'est jamais publiée ; une source n'en sort
qu'individuellement et sur déverrouillage nommé du mainteneur. Elle est consignée
en ADR. Elle concerne ce chantier de deux façons : le cas de référence de
l'épreuve est écrit et non extrait, et aucun rapport n'entre dans un chemin
versionné.
