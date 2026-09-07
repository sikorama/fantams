# Les coutures de la chaîne : assembleur, linker, builder

> **Document de conception, rien n'est écrit.** Le §10 de
> [spec-chaine-outils.md](spec-chaine-outils.md) dit *quoi* découper et par quels
> étages. Celui-ci dit *où passent les interfaces* et *ce qui doit rester
> derrière* — la question qui décide si le découpage donne trois modules profonds
> ou trois passe-plats et un CLI qui recolle.

**Vocabulaire.** *Module* : tout ce qui a une interface et une implémentation, à
n'importe quelle échelle. *Interface* : tout ce qu'un appelant doit savoir pour
s'en servir juste — la signature, mais aussi les invariants, les modes d'erreur,
l'ordre des appels, ce qu'il doit calculer lui-même. *Couture* : l'endroit où
cette interface vit, et où l'on peut changer le comportement sans éditer là.
*Profondeur* : le levier de l'interface — combien de comportement un appelant
atteint par unité d'interface apprise. Un module est **profond** quand beaucoup
d'implémentation tient derrière une petite interface, **plat** quand son
interface coûte presque aussi cher que son implémentation. *Adaptateur* : une
chose concrète qui remplit une couture.

---

## 1. Où sont les coutures aujourd'hui

Trois, dont une n'existe pas :

| couture | forme actuelle | verdict |
|---------|----------------|---------|
| source → assemblage | `asmb::Output assemble(lines)` | profonde côté appel, **large côté résultat** |
| assemblage → placement | *absente* — tenue par `asm_main.cpp` et par `bankOf()` | à créer |
| image → conteneur | `sna::build(image, opt, base, coverage, dumpKo)` | **plate** |

**L'assembleur est profond par l'entrée.** Une fonction, une liste de lignes déjà
préprocessées, et derrière : `pp`, `z80`, `keywords`, `parser`, `expr`, les deux
passes, la coverage, les recouvrements. C'est exactement la forme à préserver.

**Il est large par la sortie.** `asmb::Output` porte onze champs et **trois
représentations des mêmes octets** — `bin` + `loadAddress`, `image`, `coverage` —
plus les symboles deux fois (`symbols` et `symbolTable`). L'appelant doit savoir
que `image` est huit banques de 16 K à plat, que `coverage` ne couvre que les
64 K de base, et que `banksWritten` sert à décider une taille de dump. Ces trois
faits ne sont pas des faits d'assemblage : ce sont des **décisions de linker
posées dans l'interface de l'assembleur**.

**Le linker n'est pas absent : il est dispersé.** Son travail est fait à trois
endroits qui ne savent pas qu'ils le font — `bankOf()` qui dérive la banque de
l'adresse, `kFlatBanks = 8` qui décide qu'au-delà on ne sait pas ranger, et les
lignes d'`asm_main.cpp` qui parcourent `banksWritten` pour choisir `dumpKo`.

**`sna::build` est le module plat du lot.** Cinq paramètres dont deux pointeurs
optionnels, une contrainte non exprimable dans la signature (« une base sans
coverage écrase tout, l'appelant *doit* la fournir »), et un `dumpKo` que
l'appelant calcule. On apprend cinq choses pour en obtenir une. L'ADR 0007 a déjà
dit qu'un backend doit rendre des artefacts nommés et non un `vector<uint8_t>` ;
c'est la même couture, vue par le contenu plutôt que par la profondeur.

---

## 2. Les trois coutures visées

Une couture par maillon, une fonction par couture, et **rien à séquencer chez
l'appelant**.

### 2.1 Assembleur : `Object assemble(lines)`

Le résultat cesse d'être une image et devient un objet : les quatre blocs du
§4.6, et eux seuls.

```cpp
namespace asmb {

struct Section {
    std::string name;
    enum Kind { Ro, Rw, Uninit } kind;
    int64_t maxSize = -1;            // declare (§4.1), -1 si absent
    int64_t size = 0;                // la SEULE information d'une section Uninit
    std::vector<uint8_t> bytes;      // vide si Uninit
    std::vector<uint8_t> coverage;   // meme longueur que bytes
};

struct Symbol {
    std::string name;                 // qualifie et mangle
    enum Scope { Local, Public, Extern } scope;
    int section = -1;                 // -1 : constante, elle n'habite nulle part
    int64_t offset = 0;               // ou la valeur, pour une constante
    std::string file; int line;       // provenance, avant preprocesseur
};

struct Reloc {                        // ce que l'assembleur ne PEUT pas resoudre
    int section; int64_t offset;
    enum Kind { Abs16, Rel8, BankOf } kind;
    std::string symbol; int64_t addend;
};

struct LiteralAccess {                // §4.6 bloc 4 : on CONSIGNE, on n'interprete pas
    int section; int64_t offset;
    enum Dir { MemRead, MemWrite, PortIn, PortOut } dir;
    int64_t address;
};

struct Object {
    std::vector<Section> sections;
    std::vector<Symbol> symbols;
    std::vector<Reloc> relocs;
    std::vector<LiteralAccess> literals;
    std::string entry;                // le symbole de RUN, resolu par le linker
    bool ok; std::vector<Diagnostic> errors, warnings, prints;
};

Object assemble(const std::vector<SourceLine> &lines);
}
```

Ce que cette interface **retire** à l'appelant, et c'est là que se mesure le
gain : plus d'`image` plate, plus de `dumpKo` à dériver, plus de `loadAddress`
(un placement), plus de `runAddress` (une adresse, donc un placement — `entry`
est un *nom*). Onze champs deviennent six, dont trois sont des diagnostics.

### 2.2 Linker : `Image link(objects, script, profile)`

```cpp
namespace lnk {

struct Block {                        // une section PLACEE
    int component;                    // indice dans profile.components
    int64_t offset;                   // dans ce composant
    std::vector<uint8_t> bytes, coverage;
    std::string section;              // pour les diagnostics
};

struct Image {
    std::vector<Block> blocks;
    std::map<std::string, int64_t> symbols;   // resolus, adresses definitives
    int64_t entry = 0; int entryComponent = 0;
    bool ok; std::vector<Diagnostic> errors, warnings;
};

Image link(const std::vector<asmb::Object> &objects,
           const Script &script, const Profile &profile);
}
```

`Profile` est **une donnée, pas une hiérarchie de classes.** Les composants
matériels (RAM de base, tranches étendues, ROM basse, ROM haute, ROM Multiface de
8 K), leurs fenêtres, leurs valeurs de commutation : un profil lu depuis un
fichier. Une classe `Target` avec des méthodes virtuelles ferait une couture de
code là où seule la donnée varie, et rendrait chaque ajout de machine un
recompilage. C'est aussi ce que dit le §11 : le matériel est écrit dans le
profil, un seul endroit, vérifiable.

C1 et C2 du §10 — placer/calculer, puis vérifier — **ne sont pas deux appels.**
C2 ajoute des diagnostics au même `Image`, il n'ajoute pas de fonction. Ce sont
des coutures **internes**, privées à l'implémentation du linker et utilisables
par ses propres tests ; elles ne montent pas dans l'interface. Sans quoi
l'appelant devrait savoir qu'après `place()` il faut appeler `check()`, et l'étage
C1 livré seul figerait cette obligation dans tous les appelants.

### 2.3 Builder : `std::vector<Artifact> package(image, container)`

```cpp
namespace pack {
struct Artifact { std::string path; std::vector<uint8_t> bytes; };
std::vector<Artifact> package(const Image &img, const Container &c);
}
```

Une liste, parce que le binaire brut, le binaire AMSDOS et le DSK produisent
plusieurs fichiers (ADR 0007) ; un `path` et non un `name`, parce que le CRO
porte une arborescence. Le SNA reste l'asymétrie assumée de l'ADR 0007 : il ne
reçoit pas des morceaux mais l'image entière — ce que cette signature lui donne
déjà, sans cas particulier dans l'interface.

Les `sna::Options` actuelles — `pc`, `sp`, `cpcType` — ne sont pas des options
d'appel : ce sont des faits de conteneur, ils partent dans `Container`. `pc` vient
de `img.entry` et disparaît. `dumpKo` se dérive des `blocks` à l'intérieur.

---

## 3. Le test de suppression

Supprimer le module et regarder si la complexité disparaît (c'était un
passe-plat) ou reparaît chez N appelants (il gagnait sa place).

- **Assembleur** — reparaît en entier, partout. Acquis.
- **Linker** — reparaît chez tous les appelants : chacun redérive les banques, la
  taille du dump, les chevauchements. C'est *exactement* ce qui se passe
  aujourd'hui, et la preuve que la couture manque.
- **Builder** — reparaît par format, six fois, avec les découpages incohérents
  que l'ADR 0007 décrit.
- **`Object` sérialisé** — ne reparaît **nulle part** tant qu'il n'y a qu'un
  producteur et un consommateur dans le même processus. C'est un passe-plat à
  l'étage A, et une couture réelle à l'étage B.

## 4. Une couture par variation réelle

Un adaptateur, c'est une couture hypothétique. Deux, c'est une couture réelle.

| couture | adaptateurs | verdict |
|---------|-------------|---------|
| conteneur | brut, SNA (existants), DSK, CDT, CPR, CRO | **réelle**, et déjà à deux |
| profil de cible | CPC de base, 6128 étendu, ROM, Plus, autres machines (§13) | **réelle** — mais elle varie par la *donnée* |
| valeur `Object` en mémoire | assembleur → linker | **réelle**, un seul producteur suffit à la justifier : c'est la surface de test |
| **format de fichier objet** | fantams seul, jusqu'à l'étage D (`.rel` SDCC) | **hypothétique aujourd'hui** |

La conséquence pratique est un piège à ne pas tomber dedans : **ne pas inventer
un format de fichier objet à l'étage A.** La `struct Object` suffit, elle se teste
mieux, et sa sérialisation est une décision de l'étage B — celui qui apporte
réellement un second producteur et une compilation séparée.

## 5. Ce qui doit rester *derrière* chaque interface

La liste est la partie opérationnelle de ce document ; chaque ligne est un fait
qu'un appelant ne doit pas avoir à connaître.

**Derrière l'assembleur** : le découpage en deux passes, l'invariant qui le rend
possible (la taille d'une instruction dépend du *type* des opérandes), le mangling
des labels d'expansion, la mesure d'un bloc `BOUNDARY`, le coût en cycles de
chaque instruction.

**Derrière le linker** : `bankOf()` et toute dérivation banque↔adresse, la
géométrie des fenêtres, `RMR`/`RMR2`/`11pppccc`/`&DF00`, le choix 64 K ou 128 K,
la concaténation des sections `"ro"` pour remplir une ROM, la compression, la
vérification de continuité et ses trois pointeurs.

**Derrière le builder** : le RLE `0xE5 <compte> <valeur>` et son échappement
`0xE5 0x00`, l'en-tête AMSDOS de 128 octets, les 8 caractères d'un nom AMSDOS, la
dérivation d'un nom de morceau depuis (banque, adresse logique), l'en-tête
matériel du SNA et la fusion avec une base par la coverage.

**Ce qui ne doit surtout pas y rester** : rien de tout cela n'a de raison de
remonter dans l'interface d'un autre maillon. Le seul champ à surveiller est la
coverage, qui traverse les trois — elle *doit* traverser (elle porte l'information
que l'image seule ne porte pas, ADR 0012), mais elle voyage **attachée à ses
octets**, dans `Section` puis dans `Block`, jamais en paramètre optionnel
parallèle comme aujourd'hui. C'est la correction de fond de `sna::build`.

## 6. Le seul point qui traverse : `expr`

Le §10 le nomme comme le morceau qui ne se découpe pas. En termes de couture :
aujourd'hui `expr::Resolver` rend `bool(name, double&)` et une valeur est un
nombre. Une valeur relocalisable est un couple `(section, offset)`, et
l'affinité — `label2 - label1` absolu si même section, `label` seul
relocalisable, `label * 2` illégal en contexte relocalisable — est une propriété
de **l'arbre d'expression**, pas du symbole.

Elle doit donc être calculée dans `expr`, et non par un appelant qui
pré-classerait les symboles : l'assembleur ne peut pas savoir que `label * 2` est
illégal sans parcourir l'arbre. Et elle peut y être calculée **sans élargir
l'interface** : `eval` reste une fonction, `Result` gagne un champ de section, le
`Resolver` rend une valeur au lieu d'un `double`. Une interface presque
inchangée, une implémentation nettement plus grosse — c'est l'approfondissement,
au sens strict.

C'est aussi ce qui explique que l'étage B soit indivisible : il n'ajoute pas une
couture, il change le type qui circule à travers celle d'`expr`.

## 7. Les étages, relus comme des déplacements de couture

| étage | ce que la couture devient |
|-------|---------------------------|
| **A** | `Space` indexé par banque → `Section` nommée et typée. L'interface d'`assemble` change de *forme* mais pas de *taille*, et rien ne sort du module. Autonome, comme dit le §10. |
| **B** | le type qui traverse `expr` change ; `Object` devient sérialisable. Le gros morceau, et le seul. |
| **C1** | la couture `link` naît. Le travail dispersé dans `asm_main.cpp` et `bankOf()` migre derrière elle. |
| **C2** | rien ne bouge dans l'interface : des diagnostics de plus dans `Image`. C'est ce qui rend l'étage livrable séparément *sans* promesse anticipée. |
| **D** | le *format* objet devient une couture réelle : deux producteurs. |

## 8. La testabilité découle du découpage

Chaque couture est une surface de test, et les trois sont des fonctions pures
sans entrées-sorties — l'invariant de l'ADR 0001 tient sans effort
supplémentaire.

- `assemble` : source → `Object`. On assemble un texte et on inspecte des
  sections, des symboles et des relocs. Aucun octet placé, donc aucun test qui se
  casse quand un placement change.
- `link` : `Object` fabriqué à la main → `Image`. **C'est le gain de test
  décisif** : vérifier un chevauchement inter-banques ou une faute de continuité
  ne demande plus d'écrire un source Z80 qui la provoque, mais deux structures de
  dix lignes.
- `package` : `Image` fabriquée à la main → artefacts. Un octet de conteneur se
  teste sans assembler quoi que ce soit.

La règle qui va avec : pour tester le linker sans matériel, on **remplace le
profil** — c'est une donnée — et non pas on n'empile une couche de simulation
au-dessus. Un profil de test à deux composants de 16 K est un adaptateur légitime
de la même couture que le profil CPC, pas un faux.
