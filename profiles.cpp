// profiles.cpp - Les profils LIVRÉS, embarqués dans le binaire (fantams)
//
// **Ce fichier est une DONNÉE, pas du code.** C'est le seul du linker où un nom
// de machine apparaît, et c'est ce qui rend l'invariant du §13.2 vérifiable :
// `tests/no_machine_names.sh` interdit tout nom de machine partout ailleurs.
//
// Le texte ci-dessous est celui que l'analyseur lit, et celui que
// `--dump-profile` rend. Il n'existe aucun écrivain qui pourrait en diverger, et
// c'est tout l'objet de la décision D1 de `spec-etage-c1.md` : un porteur, un
// analyseur, un jeu de valeurs.
//
// Les commentaires en font partie. Un profil sans ses citations est un profil
// que personne ne peut auditer, et le §12.3 fait de la distinction *attesté par
// la documentation* / *mesuré ici* le cœur de son argument.
#include "profile.h"

namespace profile {
namespace {

// Amstrad CPC 6128 — 64 K de base et 64 K étendus, tels que la machine les a.
//
// Toutes les valeurs viennent de `docs/recherche/cpc-gate-array-rmr.md`, qui les
// arbitre sur sources primaires. Ce qui n'y est pas tranché est signalé ICI,
// dans le texte, et non dans une note de bas de page.
const char *kCpc6128 = R"PROFILE(// Amstrad CPC 6128
//
// Valeurs verifiees dans docs/recherche/cpc-gate-array-rmr.md, sur sources
// primaires. Statut de chaque valeur : ATTESTE = concordance de sources
// documentaires ; NON TRANCHE = les sources se contredisent, et aucune mesure
// sur machine ne les a departagees dans ce depot.

TARGET cpc6128

// --- Les fenetres : une grille de quatre, de 16 K -------------------------
// ATTESTE. [S968] §2.5 : les quatre plages #0000, #4000, #8000, #C000.
WINDOW w0 [0x0000..0x3FFF]
WINDOW w1 [0x4000..0x7FFF]
WINDOW w2 [0x8000..0xBFFF]
WINDOW w3 [0xC000..0xFFFF]

// --- Les banques : taille declaree, attributs portes ici ------------------
// ATTESTE. Huit blocs de 16 K, numerotes 0..7 : 0-3 = les 64 K de base,
// 4-7 = la page etendue ([S968] §2.5, [CW-GA], [CT-MEM] concordants).
//
// VIDEO sur les seules banques de base, et c'est un REFUS DE PLACEMENT :
// [GRIM-GA] « The extended RAM can only be used by the CPU! It is not
// possible to use it as video RAM » ; [CW-GA] « The Video RAM is always
// located in the first 64K ». Aucune section ecran ne peut vivre en RAM
// etendue.
//
// STORE : le numero sous lequel la machine designe elle-meme ses blocs.
// ATTESTE. [S968] §2.5 : « 8 16K blocks, numbered 0..7 », 0-3 = base,
// 4-7 = page etendue. C'est ce numero que --sym imprime dans sa colonne
// `store`, et celui qu'un dump plat de 128 K sait porter. Les deux ROM sont
// au-dela : elles se decrivent, et aucun conteneur de cet etage ne les sort.
BANK base0..base3  SIZE 0x4000  rw  VIDEO  STORE 0..3
BANK ext0..ext3    SIZE 0x4000  rw         STORE 4..7
BANK rom_lo        SIZE 0x4000  ro         STORE 8
BANK rom_hi<n>     SIZE 0x4000  ro         STORE 9

// --- Les configurations : les huit organisations reellement atteignables --
// ATTESTE, cinq sources concordantes et aucune divergence ([S968] §2.5 et
// appendice E, [CW-GA], [GRIM-GA], [CT-MEM], [LOGON35]).
//
// ccc = 000 est la disposition LINEAIRE PAR DEFAUT — extension presente ou
// pas. C'est la correction que le §7 de la spec a du s'appliquer : il y
// lisait « aucune RAM etendue connectee », ce que les sources refutent.
CONFIG SET ram {
    linear    [CODE %000]      { w0 base0  w1 base1   w2 base2  w3 base3 }
    ext_high  [CODE %001]      { w0 base0  w1 base1   w2 base2  w3 ext3  }
    all_ext   [CODE %010]      { w0 ext0   w1 ext1    w2 ext2   w3 ext3  }
    shifted   [CODE %011]      { w0 base0  w1 base3   w2 base2  w3 ext3  }
    ext_w1<b> [CODE %100 | b]  { w0 base0  w1 ext<b>  w2 base2  w3 base3 }
}

// `all_ext` bascule LES QUATRE fenetres d'un coup, donc aussi la pile : c'est
// la contrainte que l'etage C2 verifiera. Elle ne s'ecrit pas encore ici, et
// ce silence est nomme plutot que subi.

// Le port du PAL, et la valeur 11pppccc.
//
// NON TRANCHE — le decodage du port : A15 = 0 seul, ou A15 = 0 ET A14 = 1
// selon les sources, l'une d'elles se contredisant d'une page a l'autre.
// Sans consequence si l'on ecrit sur &7F00, et c'est ce qui est fait ici.
//
// ATTESTE, et c'est ce qui rend `out (c), c` legitime ICI : les bits bas de
// l'adresse sont INDIFFERENTS, et la valeur est la donnee ecrite, non
// l'octet bas de l'adresse. [CW-MEMEXP] intitule le registre « Port 7Fxxh »
// — le `xx` est l'attestation ; [CT-IOPORD] : « RAM Configuration | Write
// Only | b15 = 0 », tous les autres bits indifferents ; [GRIM-IO] donne le
// masque `PAL | W | &7F00 | 0 xxxxxxx xxxxxxxx`. D'ou la forme la plus
// courte, `ld bc, __port_ram_<cle> | __val_ram_<cle>` puis `out (c), c` : la
// valeur part sur le bus de donnees, et le sosie qu'elle laisse sur A7..A0
// ne selectionne rien. Sur un port dont l'octet bas COMPTE — &7FFD sur ZX,
// &243B sur Next —, cette forme serait fausse, et c'est pourquoi le fait est
// ecrit ici plutot que suppose dans les sources.
//
// NON TRANCHE — le nombre de bits de page reellement decodes : 2 (256 K),
// 3 (512 K), ou zero sur un 6128 nu. Ce profil n'expose qu'une page, celle
// que les banques ext0..ext3 portent, ce qui ne depend d'aucune des trois
// lectures.
//
// Aucun MASK : le registre selectionne par les bits 7-6 = 11 n'appartient
// qu'a cet axe, et le §12.3 le dit « sans objet sur l'axe ram du CPC ».
SELECT ram = OUT 0x7F00, %11000000 | (PAGE << 3) | CODE

// --- Les ROM : deux axes independants, qui recouvrent en LECTURE ----------
// ATTESTE, et le piege est de NOMMAGE et non de valeur : un registre
// intitule « ROM enable register » dont les bits s'appellent « ROM
// disable ». Sept sources independantes disent toutes que 0 ACTIVE et
// 1 INHIBE, sans exception ; [S968] l'enfonce en donnant l'etat au reset :
// « the mode and ROM enable register is set to zero, ENABLING both halves ».
//
// D'ou CODE = 0 sur l'etat `on` et CODE = 1 sur `off`.
//
// Le MASK est ce qui rend ces axes utilisables : le meme registre porte le
// mode ecran (bits 1-0) et le compteur d'interruption (bit 4). Y sortir la
// seule valeur de l'axe de ROM ecraserait le mode, en silence — la faute
// exacte que le §12.3 combat.
CONFIG SET rom_lower OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo } }
CONFIG SET rom_upper OVER ram { off [CODE 1] { }  on<n> [CODE 0] { w3 rom_hi<n> } }

SELECT rom_lower = OUT 0x7F00, MASK %00000100, CODE << 2

// Deux ecritures, et la seconde porte le numero de ROM : c'est elle que le
// §12.3 nomme `__romnum_`. ATTESTE : [S968] appendice K et §10, [CT-EXPROM]
// et [GRIM-IO] concordants sur &DFxx en ecriture seule.
//
// NON TRANCHE, sans consequence ici — la plage de numeros : 0..251 est la
// limite FIRMWARE ([S968] §2.2), le materiel acceptant les 256 valeurs
// ([GRIM-GA], [CW-UROM]). Deux perimetres, pas une contradiction.
SELECT rom_upper = OUT 0x7F00, MASK %00001000, CODE << 3
                   OUT 0xDF00, MASK %11111111, PAGE

// NON TRANCHE, et hors de ce que ce profil doit dire — l'effet du bit 4 du
// meme registre : [S968] dit qu'ecrire 1 effface LE BIT DE POIDS FORT du
// diviseur par 52 ; Grimware, Cpctech et Logon disent qu'il remet LE COMPTEUR
// ENTIER a zero. Les deux ne peuvent pas etre vraies. Aucun placement n'en
// depend, mais le bit appartient au meme octet que les deux axes de ROM :
// une copie de l'etat en RAM doit le connaitre.
)PROFILE";

// Amstrad CPC 464/6128 Plus — meme RAM que le 6128, plus un axe de ROM
// nouveau : la redirection de la ROM basse par `RMR2` (etage E, ADR 0032).
//
// Toutes les valeurs viennent de `docs/recherche/cpc-gate-array-rmr.md` §D,
// qui les arbitre sur [GRIM-GA] et [CW-UROM] — la documentation ASIC
// officielle n'a pas ete lue directement (§0 du meme dossier).
const char *kCpcPlus = R"PROFILE(// Amstrad CPC 464/6128 Plus
//
// Valeurs verifiees dans docs/recherche/cpc-gate-array-rmr.md §D, sur
// [GRIM-GA] et [CW-UROM]. Statut de chaque valeur : ATTESTE = concordance de
// sources documentaires ; NON TRANCHE = non tranche dans ce depot ; HORS
// PERIMETRE = attests mais pas encore ecrit ici, et dit pourquoi.

TARGET cpcplus

// --- La RAM : IDENTIQUE au 6128, copiee et non reinventee -----------------
// ATTESTE. docs/recherche/cpc-gate-array-rmr.md §D.1 : le PAL de la RAM et
// sa table de configurations ne changent pas sur Plus — meme decodage,
// memes huit dispositions. Rien de neuf a ecrire sur cet axe.
WINDOW w0 [0x0000..0x3FFF]
WINDOW w1 [0x4000..0x7FFF]
WINDOW w2 [0x8000..0xBFFF]
WINDOW w3 [0xC000..0xFFFF]

BANK base0..base3  SIZE 0x4000  rw  VIDEO  STORE 0..3
BANK ext0..ext3    SIZE 0x4000  rw         STORE 4..7
BANK rom_lo        SIZE 0x4000  ro         STORE 8
BANK rom_hi<n>     SIZE 0x4000  ro         STORE 9

CONFIG SET ram {
    linear    [CODE %000]      { w0 base0  w1 base1   w2 base2  w3 base3 }
    ext_high  [CODE %001]      { w0 base0  w1 base1   w2 base2  w3 ext3  }
    all_ext   [CODE %010]      { w0 ext0   w1 ext1    w2 ext2   w3 ext3  }
    shifted   [CODE %011]      { w0 base0  w1 base3   w2 base2  w3 ext3  }
    ext_w1<b> [CODE %100 | b]  { w0 base0  w1 ext<b>  w2 base2  w3 base3 }
}

// Le port du PAL et de `RMR`/`RMR2` est LE MEME sur toute la famille : c'est
// ce qui rend `GA_PORT` interessant plutot qu'un triplet `__port_` par axe
// (ADR 0032, decision 1) — ecrire &7F00 quatre fois ici serait la meme
// information repetee quatre fois.
CONST GA_PORT = 0x7F00

// Aucun MASK sur cet axe : le registre selectionne par les bits 7-6 = 11
// n'appartient qu'a lui.
SELECT ram = OUT GA_PORT, %11000000 | (PAGE << 3) | CODE

// --- Les ROM classiques : les deux axes du 6128, INCHANGES -----------------
// ATTESTE. §D.1 : `RMR` (bits 7-5 = 100) est le meme registre, avec la meme
// polarite de bits (0 active, 1 inhibe). Rien de propre au Plus ici.
CONFIG SET rom_lower OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo } }
CONFIG SET rom_upper OVER ram { off [CODE 1] { }  on<n> [CODE 0] { w3 rom_hi<n> } }

SELECT rom_lower = OUT GA_PORT, MASK %00000100, CODE << 2
SELECT rom_upper = OUT GA_PORT, MASK %00001000, CODE << 3
                   OUT 0xDF00, MASK %11111111, PAGE

// --- Ce qui est PROPRE au Plus : RMR2 redirige la ROM basse ---------------
// ATTESTE. §D.1, table de [GRIM-GA] : `RMR2` est selectionne par bits 7-5 =
// 101 (contre 100 pour RMR), et ses cinq bits restants portent DEUX champs —
// LRM (bits 4-3, la fenetre) et l'ID de ROM PHYSIQUE de la cartouche (bits
// 2-0, 0..7 seulement : « you can only map the first 8 physical roms of the
// cartridge as Lower ROM », [GRIM-GA], confirme par [CW-UROM]). D'ou
// CODE = (LRM << 3) | n, verifie sur les deux exemples de [GRIM-GA] :
// `%101 00 000` = &7FA0, `%101 11 000` = &7FB8.
//
// Aucun MASK : comme pour l'axe `ram`, les bits 7-5 = 101 n'appartiennent
// qu'a ce registre — une seule ecriture le regle en entier.
//
// STORE 16, comme `rom_hi<n>` : un seul numero pour toute la famille
// PARAMETRIQUE, deplace hors de 0..9 deja pris par les blocs RAM et les deux
// ROM classiques — ce n'est PAS l'ID de ROM physique, qui reste PAGE, comme
// pour `rom_hi<n>` (C1.8). L'ID physique (0..7, ATTESTE) est ce que `--sym`
// imprime dans sa colonne `page`, pas dans `store`.
BANK crom<n> SIZE 0x4000 ro STORE 16

CONFIG SET cart_rom OVER ram {
    w0<n> [CODE %00000 | n] { w0 crom<n> }
    w1<n> [CODE %01000 | n] { w1 crom<n> }
    w2<n> [CODE %10000 | n] { w2 crom<n> }
}
SELECT cart_rom = OUT GA_PORT, %10100000 | CODE

// --- ROM physiques 8..31 : adressables SEULEMENT en ROM haute -------------
// ATTESTE. docs/recherche/cpc-gate-array-rmr.md §D.1, [GRIM-GA] : « You can
// only map the first 8 physical ROMs of the cartdridge as Lower ROM. To
// access physical ROM above 7, you have to use the Upper ROM mapping. »
// Confirme par [CW-UROM] : « The first 8 physical roms can be accessed as
// lower roms. And all the 32 physical roms can be accessed as upper roms. »
//
// Meme port &DF00 que `rom_upper`, mais UNE SEMANTIQUE DIFFERENTE sur Plus,
// selon le bit 7 de l'octet ecrit ([GRIM-GA], meme paragraphe ; confirme mot
// pour mot par [CW-UROM]) :
//   bit7=0, bits4-0 = L : ID logique 0..127 — c'est ce que `rom_upper` ecrit
//                          deja pour `rom_hi<n>` (une ROM D'EXTENSION, pas de
//                          cartouche).
//   bit7=1, bits4-0 = P : ID PHYSIQUE de cartouche 0..31 — bits6-5 ignores.
// D'ou `0x80 | PAGE` plutot que `PAGE` seul.
//
// `crom<n>`, la MEME banque (STORE 16) que `cart_rom`, et non `rom_hi<n>` :
// c'est ce qui fait qu'une section placee ici finit dans le meme chunk que
// `cpr::build` (qui ne lit que STORE 16), quelle que soit la fenetre par
// laquelle son contenu a ete ecrit — bas via `cart_rom`, haut via cet axe.
CONFIG SET cart_rom_hi OVER ram {
    off   [CODE 1]  { }
    on<n> [CODE 0]  { w3 crom<n> }
}
SELECT cart_rom_hi = OUT GA_PORT, MASK %00001000, CODE << 3
                      OUT 0xDF00, MASK %11111111, 0x80 | PAGE

// HORS PERIMETRE — deux choses attestees par §D, non ecrites ici, et dit
// pourquoi plutot que subi :
//
// 1. La disposition LRM = 11, qui mappe EN PLUS la page E/S de l'ASIC en w1.
//    Une page d'E/S n'est pas une banque de memoire adressable par une
//    SECTION : la representer demanderait un mot de vocabulaire de profil
//    que rien d'autre ne consomme encore (meme raison que les macros de
//    profil, ADR 0032, "ce que cet ADR ne decide pas").
// 2. Le deverrouillage de l'ASIC, sans lequel RIEN de ce qui precede n'a
//    d'effet : c'est un etat d'EXECUTION que rien au linkage ne peut
//    verifier (ADR 0032, docs/recherche/cpc-gate-array-rmr.md §D.2). Un
//    programme qui n'utilise aucun de ces axes doit garder le bit 5 a 0
//    dans toutes ses ecritures a GA_PORT, faute de quoi il touchera RMR2 si
//    l'ASIC se trouve deverrouille par ailleurs.
//
// NON TRANCHE, herite du 6128 sans changement — RMR est le meme registre :
// l'effet du bit 4 (compteur d'interruption) reste contradictoire entre
// [S968] (efface le seul bit de poids fort) et [GRIM-GA]/[CT-GAINT]/
// [LOGON35] (remet le compteur entier a zero). Aucun placement n'en depend.
)PROFILE";

struct Builtin { const char *name; const char *text; };
const Builtin kBuiltins[] = {
    {"cpc6128", kCpc6128},
    {"cpcplus", kCpcPlus},
};

} // namespace

std::string builtin(const std::string &name) {
    for (const Builtin &b : kBuiltins)
        if (name == b.name) return b.text;
    return std::string();
}

std::vector<std::string> builtinNames() {
    std::vector<std::string> v;
    for (const Builtin &b : kBuiltins) v.push_back(b.name);
    return v;
}

} // namespace profile
