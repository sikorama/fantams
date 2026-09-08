# Docs de domaine

Comment les skills doivent consommer la documentation de domaine de ce dépôt
quand ils explorent le code.

## À lire avant d'explorer

- **`CONTEXT.md`** à la racine : le glossaire. Ce n'est ni une spec ni un
  bloc-notes — uniquement des termes et leurs définitions.
- **`docs/adr/`** : les ADR qui touchent la zone où l'on va travailler.

Ce dépôt est à **contexte unique** : il n'y a pas de `CONTEXT-MAP.md` et il n'y
en aura pas tant que le dépôt reste un seul module.

## Structure

```
/
├── CONTEXT.md
├── docs/adr/
│   ├── 0001-coeur-pur-et-adaptateurs-fins.md
│   └── …
└── *.cpp, *.h          ← racine plate : le cœur, plus les deux outils
```

## Employer le vocabulaire du glossaire

Quand une sortie nomme un concept du domaine — titre d'issue, proposition de
refactorisation, hypothèse, nom de test —, employer le terme **tel que
`CONTEXT.md` le définit**. Ne pas dériver vers les synonymes que le glossaire
range explicitement sous `_Éviter_` : ils y sont parce qu'ils ont déjà coûté
quelque chose.

Si le concept nécessaire n'est pas encore au glossaire, c'est un signal : soit
on invente une langue que le projet n'emploie pas — à reconsidérer —, soit il y
a un vrai manque, à noter pour `/domain-modeling`.

## Signaler les conflits avec un ADR

Si une sortie contredit un ADR existant, le dire explicitement plutôt que de
passer outre en silence :

> _Contredit l'ADR 0007 (pipeline d'export en trois couches), mais mérite d'être
> rouvert parce que…_

Deux points propres à ce dépôt, à connaître avant de proposer quoi que ce soit :

- Plusieurs ADR portent un encadré de correction daté d'une étape ultérieure.
  Cet encadré fait autorité sur le corps du document.
- Un ADR en `status: proposed` décrit une forme visée, pas le code existant.
  L'ADR 0007 le dit de lui-même : « rien de cet ADR n'est écrit ».
