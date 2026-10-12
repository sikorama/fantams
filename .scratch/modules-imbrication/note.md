# Modules : imbrication (à investiguer, en attente de cas d'usage concrets)

Aujourd'hui `MODULE x` remplace le module actif, il ne s'empile pas (docs/syntax.md, pp.cpp).
Un utilisateur a suggéré des namespaces (`NS`/`ENDNS`) pour préfixer des structures dans un module.

Pistes :
- Imbrication par pile : `MODULE chose` dans `toto` donne `toto.chose.ADDR`, un ENDMODULE par niveau.
  Préféré à un second mécanisme `NS`.
- Champs : jamais préfixés par le module, coupés au dernier point (`toto.chose.titi` + `bidule`).
- Vraie ambiguïté seulement en cas de collision module/structure/label sous le même préfixe
  (existe déjà : module `gfx` + structure `gfx`). Piste : refuser à la définition, diagnostic nommant les deux.
- Si on garde le switch : avertir quand un MODULE s'ouvre alors qu'un autre est actif.
- À faire en même temps : ADR « les modules » (aucun n'existe) et retirer de pp.cpp
  (~1103, 1555, 1572) les mentions de l'assembleur de référence (cf. ADR 0025).

À vérifier : comportement actuel du code en cas de collision module/structure.
Déclencheur : un cas d'usage réel.
