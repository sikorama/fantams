---
status: accepted
---

# Le port de commutation devient une constante `CONST` du profil ; l'axe sort de la clé de `__val_` (pas de `__mask_`) ; le rang de `__port_` reste ouvert

[[port-et-valeur-a-revoir-au-second-profil]] (mémoire de chantier) et
`docs/etage-c1.md` §C1.7 posaient trois questions et les réservaient
explicitement au moment où un second profil arriverait. Le CPC+ est ce second
profil. Cet ADR les reprend une à une — et refuse de trancher la troisième en
disant pourquoi.

## Contexte

`profiles.cpp` porte déjà, pour le seul CPC 6128, **trois axes qui partagent un
seul port** : `ram`, `rom_lower`, `rom_upper` écrivent tous les trois sur
`0x7F00` (`SELECT ram = OUT 0x7F00, …`, `SELECT rom_lower = OUT 0x7F00, MASK
…`, `SELECT rom_upper = OUT 0x7F00, MASK …`). Le §12.3 en tire un symbole par
axe et par section — `__port_ram_audio`, `__port_rom_lower_x`… — dont
`docs/etage-c1.md` §C1.7 note déjà que, sur ce triplet, « un symbole sur trois
porte de l'information » : le port ne varie jamais, seule `__val_` bouge avec
le placement.

Le CPC+ (`docs/recherche/cpc-gate-array-rmr.md` §D) ajoute une quatrième
écriture de commutation, `RMR2` — la redirection de la ROM basse et de la page
E/S ASIC — et c'est **le même port, `0x7F00`**, sélectionné par un bit du même
octet (bit 5). Ce n'est pas une coïncidence : `RMR`, `RMR2` et le PAL de la RAM
sont décodés par le même Gate Array / ASIC, à la même adresse recommandée.

## Décision 1 — `CONST` : le port sort du triplet, il devient une constante du profil

Un profil peut désormais déclarer `CONST <nom> = <expr>` : une constante
littérale, calculée au moment où le profil est chargé (avant tout placement),
et rendue au source exactement comme les symboles de commutation — le CLI la
passe à l'assembleur, qui ne connaît toujours aucune machine. `--dump-profile`
la réimprime : c'est une donnée du profil, pas un mécanisme séparé, et
`no_machine_names.sh` continue de tenir puisque `CONST` ne nomme rien, il
route une valeur déjà présente dans le texte du profil.

`profiles.cpp` déclare `CONST GA_PORT = 0x7F00` une fois, dans le profil
CPC 6128 comme dans le profil CPC+ : la même valeur, parce que c'est le même
composant. Les trois `SELECT` existants et le futur `SELECT lrom2` du CPC+
n'écrivent plus un port en propre — ils l'écrivent tous à `GA_PORT`, un seul
domicile.

**Ce que ça remplace, et ce que ça ne remplace pas.** `__port_<axe>_<clé>`
disparaît des programmes qui adoptent `GA_PORT` : c'était le nombre sans
information du triplet, maintenant à un seul endroit et sous un nom court. Le
mécanisme `__port_` lui-même n'est pas retiré du langage — un profil dont le
port varierait réellement par axe continuerait à l'émettre, faute de mieux ; ce
n'est simplement pas le cas du CPC ni du CPC+, sur aucun de leurs axes connus.

## Décision 2 — l'axe sort de la clé de `__val_`, et RESTE dans celle de `__mask_`

`__val_ram_audio` devient `__val_audio`. Argument structurel, pas empirique :
une section est placée par **l'état d'un seul axe** — le modèle du profil
(`profile.h`, `Axis`/`State`/`Slot`) n'offre aucune façon d'attacher une
section à deux axes à la fois, et `OVER` (une ROM qui recouvre une RAM en
lecture) échange des *banques* derrière une fenêtre, jamais deux sections sous
un même nom. Les noms de section sont par ailleurs uniques dans tout le
programme lié (C1.0 : la fusion par nom est globale, pas par axe). La clé ne
peut donc jamais collisionner en retirant l'axe, sur aucun profil examiné — les
trois axes du CPC 6128 le montraient déjà, et l'axe `lrom2` du CPC+ n'est qu'un
quatrième exemple de la même forme.

**Amendement, trouvé en préparant E2, avant tout code.** `__mask_<axe>` (et
`__mask2_<axe>`) n'ont **aucune clé de section** — `link.cpp` les offre par
`offer("__mask_" + axisName, mask)`, l'axe seul. L'argument ci-dessus repose
sur l'unicité du **nom de section**, qui n'existe pas ici : le masque
appartient à l'axe, pas à une section placée. Le retirer collapserait
`__mask_ram`, `__mask_rom_lower` et `__mask_rom_upper` — trois masques
distincts, déjà coexistants sur le seul CPC 6128 — vers le même nom nu
`__mask`, ambigu par construction. `__mask_`/`__mask2_` **gardent donc leur
axe**, et rejoignent `__port_`/`__port2_`/`__romnum_` dans ce que cet étage ne
renomme pas.

Ce n'est donc pas le CPC+ qui a permis de trancher `__val_` : c'est le fait
d'avoir, dans un seul profil, plus d'un axe — vrai depuis `rom_lower` et
`rom_upper`, et resté non tranché seulement parce que personne n'avait encore
eu besoin d'écrire un second profil pour s'en assurer.

## Décision 3 — le rang de `__port_` reste ouvert, et le CPC+ ne peut pas le trancher

La question posée par [[port-et-valeur-a-revoir-au-second-profil]] est «
`__port_` est-il, **partout**, un invariant par cible ? ». Le CPC+ ne répond
pas à cette question : `RMR2` est décodé par **le même composant physique** que
`RMR` et le PAL — même famille, même câblage B/C que celui déjà cité comme cas
particulier en C1.7. Ajouter un profil qui réutilise le même port n'apporte
aucune donnée sur une architecture qui câblerait ses axes différemment ; ça
confirme seulement, une quatrième fois, ce qu'on savait déjà du CPC.

**Cette question reste donc ouverte**, jusqu'à ce qu'un profil hors famille CPC
existe — le ZX Next annoncé dans [[verification-sur-machine-reelle]], ou un
MSX. Ne pas la retrancher à cette occasion serait répéter l'erreur que C1.7
signalait déjà : décider d'après le cas particulier.

## Ce que cet ADR ne décide pas

**Les macros de profil** — un cran au-dessus d'`CONST`, réservées par
`docs/etage-c1.md` §C1.7 à « quand deux profils seront sur la table » pour la
question de savoir jusqu'où le profil va sans devenir du code. Un profil,
`CONST` seul, reste. **Le contenu du profil CPC+ lui-même** — fenêtres,
banques, `CONFIG SET`, l'axe `lrom2` — est écrit à la suite de cet ADR, pas
ici. **Le verrou ASIC** n'est pas un axe du profil : `docs/recherche/cpc-gate-array-rmr.md`
§D.2 explique pourquoi — c'est un état d'exécution, dont rien au moment du
linkage ne peut savoir ce qu'il vaut, donc rien qu'un profil puisse
représenter comme un `SELECT`.
