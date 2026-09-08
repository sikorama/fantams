---
status: accepted
---

# Le corpus ne se publie que pièce par pièce, sur déverrouillage nommé

Le corpus est conservé hors de fantams, et **son intégralité ne doit jamais être
publiée** : c'est l'ensemble qui est le secret, pas chaque pièce. Une source
n'en sort qu'**individuellement**, et seulement après que le mainteneur a
constaté qu'elle est **déjà publique ailleurs**. La forme opératoire est
dissymétrique : un contributeur — humain ou agent — **propose**, le mainteneur
**déverrouille nommément**. Jamais l'inverse.

## Contexte

Le corpus sert à mesurer la diversité syntaxique, et cette mesure a besoin de
sources réelles que leurs auteurs n'ont pas toutes rendues publiques. La règle
n'est donc ni une précaution juridique ni une pudeur : c'est la condition à
laquelle l'instrument de mesure peut exister.

Le risque n'est pas le mainteneur, qui connaît la contrainte. C'est l'agent
serviable qui versionne un extrait de corpus pour faire passer un test, ou le
script de mesure qui écrit un rapport dans un chemin suivi. Les deux sont des
gestes normaux ailleurs, et destructeurs ici.

## Ce que la règle couvre, au-delà du texte des sources

Un rapport de comparaison a déjà été publié dans l'historique des deux dépôts —
`compare/last-report.json` dans fantams, `fantams/compare/last-report.json` dans
z80live-lite, retrouvable par `git log --diff-filter=A` sur ces chemins. Il ne
contenait aucun corps de source, mais les **noms** de deux cents sources et,
dans les messages de diagnostic recopiés tels quels, des **noms de symboles
internes** et de courts fragments de texte source.

D'où deux conséquences sur la forme des rapports : ils portent un identifiant
opaque plutôt qu'un nom, et un **compte par catégorie d'erreur** plutôt que le
texte des diagnostics. Le texte d'un diagnostic cite la source ; c'est sa
fonction, et c'est pourquoi il ne peut pas être publié.

## Ce que la règle ne dit pas

Elle n'interdit pas qu'un **cas de référence** dérive d'une source du corpus. La
position inverse — un cas de référence s'écrit, jamais ne s'extrait — a été
examinée et écartée : elle était surtout plus commode à faire respecter, et
priver les cas de référence des constructions réellement rencontrées coûte plus
que le confort d'une règle absolue. Un cas de référence dérivé est donc légitime
**après** déverrouillage de la source dont il vient.

## Conséquences

L'historique publié n'est pas réécrit : la fuite est faite, et une réécriture
ne reprend pas ce qui est indexé. Le nettoyage éventuel de l'historique viendra
pour d'autres raisons, et à la fin des chantiers.

Cette règle est la raison pour laquelle `db/*.sqlite` et les rapports de
comparaison sont exclus du suivi de version côté z80live — ces lignes de
`.gitignore` sont l'effet, ce document est la cause.
