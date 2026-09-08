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
CONFIG SET rom_lower OVER ram { off [CODE 1] { }  on [CODE 0] { w0 rom_lo    } }
CONFIG SET rom_upper OVER ram { off [CODE 1] { }  on [CODE 0] { w3 rom_hi<n> } }

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

struct Builtin { const char *name; const char *text; };
const Builtin kBuiltins[] = {
    {"cpc6128", kCpc6128},
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
