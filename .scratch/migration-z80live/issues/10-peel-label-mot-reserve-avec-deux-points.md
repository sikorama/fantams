# 10: `peelLabel` rend un label pour un mot réservé suivi de deux-points

**Status:** needs-triage

**Hors périmètre du chantier « migration ».** Trou trouvé en corrigeant le
beautify (« un mot réservé ne nomme pas un label, deux-points ou pas »), qui a
été refermé *chez l'appelant* et non à la source.

## Le constat

`kw::peelLabel` a deux branches, et une seule applique l'ADR 0015 :

```cpp
if (q < code.size() && code[q] == ':') {
    label = code.substr(0, p);      // <- aucun test de mot réservé
    rest  = trim(code.substr(q + 1));
    if (sawColon) *sawColon = true;
    return;
}
std::string tok = upper(code.substr(0, p));
if (!isReservedWord(tok, ph) && ...) {   // <- ici seulement
```

Donc `peelLabel("ldi:ldi", ..., Assembly)` rend `label = "ldi"`. Or l'ADR 0015
dit qu'un mot réservé ne nomme pas un label — le deux-points n'y change rien, il
sépare deux instructions.

## Pourquoi ce n'est pas (encore) visible

L'assembleur ne voit jamais `ldi:ldi` : le préprocesseur a déjà scindé la ligne
quand `asm.cpp` la reçoit, et c'est lui qui émet l'avertissement « is read as
two statements, not as a label ». Le trou est donc masqué par l'ordre des
étages, pas comblé.

Le beautify, lui, travaille sur la source **non préprocessée** — c'est sa
définition (ADR 0013) — et il est tombé dedans en plein.

## Ce qu'il faudrait décider

Refermer le trou dans `peelLabel` demande de savoir ce que doit valoir `rest`
quand la tête est un mot réservé suivi de `:` :

- `label` vide et `rest` = la ligne entière — cohérent avec la branche sans
  deux-points, mais l'appelant perd l'information « il y avait un `:` » ;
- ou une troisième sortie qui dit « ce `:` est un séparateur », ce que le
  préprocesseur reconstruit aujourd'hui de son côté (sa liste `glued`).

La seconde ferait vivre la règle à un seul endroit au lieu de deux et demi.

## Pourquoi ce n'est pas fait ici

`peelLabel` est sur le chemin de `asm.cpp` (l. 1159), donc sur un chemin qui
**produit des octets**. Le corriger sans mesurer demanderait au minimum un
passage de la comparaison sur corpus avant/après, ce qui est le genre de
vérification qu'on ne bâcle pas en marge d'un correctif de mise en forme.

Le garde-fou posé dans `beautify.cpp` interroge `kw::isReservedWord`, donc la
règle reste **déclarée** une seule fois même si elle est **appliquée** à deux
endroits. C'est tenable, ce n'est pas satisfaisant.
