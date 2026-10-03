---
status: accepted
---

# La mesure dynamique est un outil, pas une fonction du binaire

Mesurer sur une machine émulée la durée entre deux adresses — pour caler une
trame de 19968 NOPs, ou pour compenser ce qu'on ne peut prévoir — est du
ressort d'un **script sous `tools/`**, pas du binaire `fantams`. Il pilote
AMSpiriT en lui posant deux `z80_bp` et en soustrayant les `ticks` rendus (un
tick est un NOP). `fantams` ne gagne aucune dépendance ni aucun client réseau.

## Ce qui est décidé

- **Le script** lit la table des symboles (`--sym`, ADR 0019) pour résoudre les
  labels, traduit `Bn:adresse` en `Bnn:hhhh` **à un seul endroit** (les deux
  notations ont la même graphie et pas la même sémantique ; voir
  `docs/recherche/amspirit-pilotage-api.md` §5), lance l'émulateur headless,
  injecte le livrable par `/api/media` (jamais par `/api/ram` + `/api/exec`),
  et écrit un CSV. Il localise l'émulateur par variable d'environnement ; son
  absence est une erreur claire, pas un échec obscur.
- **Un échantillon** est le dernier franchissement de `from` avant le
  franchissement de `to`. `--samples N` fixe le nombre d'échantillons ; la
  cadence est celle du programme. Les `ticks` repartent de 0 au reset dur, au
  chargement d'un SNA et au retour arrière : un échantillon qui traverse l'un
  d'eux est invalide.
- **La mesure est brute.** Une interruption tombée entre `from` et `to` gonfle
  la durée sans que l'émulateur le signale. Quand une mesure statique existe
  pour la même paire, le CSV porte une colonne `expected`, et un échantillon
  dont l'écart dépasse un seuil est marqué `suspect` — jamais supprimé.
- **La table est une sortie.** Un échantillon est une ligne du CSV (indice,
  trame, `from`, `to`, NOPs, `ticks` de départ). Elle sert à un humain ou à un
  agent, qui choisit une constante ; ou elle est transformée en `db` par un
  script puis incluse, selon la doctrine de l'ADR 0010 (générer les données avec
  un script). Un build **ne lit jamais** une mesure : la construction reste
  hermétique et reproductible.
- **Le balayage de la table de durées** (ADR 0035) est une suite d'épreuves
  optionnelle, sautée quand l'émulateur est absent. L'émulateur n'est pas une
  dépendance obligatoire de `ctest`.

## Pourquoi pas dans le binaire

L'ADR 0001 tient le cœur pur et les adaptateurs fins. Un client HTTP, des
ROM à localiser et un processus à superviser sont un adaptateur, et un
adaptateur qui ne sert qu'à une commande ne gagne rien à vivre dans le même
exécutable. Le harnais existant est déjà du bash.

## Pourquoi pas une boucle de build

Un assembleur qui lit sa propre mesure se réassemble avec une valeur qu'une
machine lui a dictée : sa sortie dépend alors d'un émulateur, de sa version et
de l'état du programme. L'ADR 0035 en tire la conséquence : la mesure
informe l'auteur, elle ne construit pas à sa place.
