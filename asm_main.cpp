// asm_main.cpp - end-to-end CLI: .asm source -> preprocessor -> assembler -> .bin
//
//   fantams (file.asm | file.fo...) [-o out] [-s] [-E] [--strict] [--beautify]
//           [--normalize] [--no-detach-labels] [--no-indent-blocks] [--base base.sna]
//     -o : output binary file (default: <source>.bin)
//          Le TYPE de sortie se deduit de l'extension, comme pour .sna :
//          « -o x.fo » assemble SEUL et ecrit l'objet, sans linker.
//          Et une entree « .fo » est un objet DEJA assemble : on le relit au
//          lieu de l'assembler. C'est ce que la compilation separee demande.
//     -s : print the symbol table
//     --base : reference snapshot the assembled bytes are laid onto (ADR 0012).
//          Only meaningful for a .sna output. Every address the source did NOT
//          write keeps the base's byte — that is what makes firmware calls work.
//     -E : write the UNROLLED source to -o instead of assembling (macros
//          expanded, loops unrolled, includes inserted, scopes renamed).
//          C'est un livrable de premier plan, pas un artefact de debogage :
//          c'est lui qui rend verifiable ce que le preprocesseur a compris.
//          La sortie est MISE EN FORME : la mise en forme fait partie de la
//          definition de la source deroulee (ADR 0013).
//     --beautify : mettre en forme le source et l'ecrire dans -o, sans
//          preprocesseur ni assemblage. C'est ce que le bouton « Mettre en
//          forme » de l'editeur appelle. Preserve le nombre de lignes (ADR 0013).
//     --strict : refuser tout ce qui n'est pas du Z80 canonique — sucre
//          un-vers-plusieurs et orthographes obsoletes (ADR 0017). N'ajoute rien,
//          refuse. C'est le drapeau du PIPELINE, pas d'une couche.
//     --no-detach-labels : garder « label: instruction » sur une seule ligne.
//     --no-indent-blocks : ne pas indenter le corps des blocs (repeat, macro,
//          if, while, for, struct). Comme le detachement, c'est un STYLE et non
//          un canon, d'ou l'opt-out (ADR 0013, regle 4).
//          Le beautify detache par defaut (regle 3) : l'indentation fixe aligne
//          tous les opcodes, un label de longueur variable ne les aligne pas.
//     --sym[=fichier] : ecrire la TABLE DES SYMBOLES (ADR 0019) : un CSV d'une
//          ligne par label et par constante, avec type, adresse logique, banque et
//          adresse de rangement, fichier et ligne D'ORIGINE (avant preprocesseur).
//          Destinee a un desassembleur ou un emulateur, pas a un humain — pour
//          l'humain, c'est « -s ». Sans « = », le chemin est derive de -o : le
//          fichier voyage a cote du binaire qu'il decrit.
//     --normalize : canoniser le source SANS le derouler (ADR 0017) : orthographes
//          obsoletes et opcodes composes. Change deliberement le nombre de lignes.
//          Transformation INDEPENDANTE du beautify, composable avec lui — les deux
//          ensemble normalisent puis mettent en forme. Incompatible avec -E, qui
//          canonise deja PUIS deroule : deux sorties differentes.
#include "asm.h"
#include "beautify.h"
#include "fo.h"
#include "profile.h"
#include "script.h"
#include "link.h"
#include "pp.h"
#include "sna.h"
#include "sym.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static bool readFile(const std::string &path, std::string &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf();
    out = ss.str();
    return true;
}

int main(int argc, char **argv) {
    std::string path, outPath, basePath;
    std::vector<std::string> inputs;
    bool showSyms = false;
    bool dumpOnly = false;
    bool beautifyOnly = false;
    bool normalizeOnly = false;
    bool wantSym = false;
    std::string symPath;
    bool strict = false;
    bool detachLabels = true;
    bool indentBlocks = true;
    // Le PROFIL DE CIBLE : un nom livre, ou un fichier. Les deux passent par le
    // meme analyseur et le meme chemin de code, et c'est ce qui prouve que le
    // texte embarque n'a aucun privilege (D3).
    std::string targetName, profilePath, dumpProfile;
    // Le SCRIPT DE LINKAGE : quelle section va ou. Il arrive avec le code qui
    // l'honore, et pas avant : un `-T` qui accepterait un script sans l'appliquer
    // laisserait croire un placement qui n'a pas eu lieu.
    std::string scriptPath;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) outPath = argv[++i];
        else if (a == "--base" && i + 1 < argc) basePath = argv[++i];
        else if (a == "-s") showSyms = true;
        else if (a == "-E") dumpOnly = true;
        else if (a == "--beautify") beautifyOnly = true;
        else if (a == "--normalize") normalizeOnly = true;
        // « --sym » ne prend pas d'argument positionnel : « fantams --sym src.asm »
        // serait ambigu (chemin de sortie, ou source ?). Le chemin explicite passe
        // par « --sym=... », le defaut se derive de -o.
        else if (a == "--sym") wantSym = true;
        else if (a.rfind("--sym=", 0) == 0) { wantSym = true; symPath = a.substr(6); }
        else if (a == "--strict") strict = true;
        else if (a == "--no-detach-labels") detachLabels = false;
        else if (a == "--no-indent-blocks") indentBlocks = false;
        else if (a == "--target" && i + 1 < argc) targetName = argv[++i];
        else if (a.rfind("--target=", 0) == 0) targetName = a.substr(9);
        else if (a == "-P" && i + 1 < argc) profilePath = argv[++i];
        else if (a == "-T" && i + 1 < argc) scriptPath = argv[++i];
        else if (a == "--dump-profile" && i + 1 < argc) dumpProfile = argv[++i];
        else if (a.rfind("--dump-profile=", 0) == 0) dumpProfile = a.substr(15);
        else inputs.push_back(a);
    }
    // --- Le profil de cible ------------------------------------------------
    // Trois formes, et la troisieme est une COPIE. `--dump-profile` ne serialise
    // rien : il rend le texte que l'analyseur lira, tel quel. C'est ce qui rend
    // impossible la divergence entre un ecrivain et un lecteur — la faute que
    // l'etape B7 a testee pour le `.fo`, et que la decision D1 evite en n'ayant
    // qu'un seul porteur.
    auto namesList = [] {
        std::string s;
        for (const std::string &n : profile::builtinNames()) {
            if (!s.empty()) s += ", ";
            s += n;
        }
        return s;
    };
    if (!dumpProfile.empty()) {
        const std::string text = profile::builtin(dumpProfile);
        if (text.empty()) {
            fprintf(stderr, "error: no built-in profile named '%s' (built-in: %s)\n",
                    dumpProfile.c_str(), namesList().c_str());
            return 2;
        }
        fputs(text.c_str(), stdout);
        return 0;
    }
    // --- Le script de linkage ----------------------------------------------
    // Il est lu AVANT le profil, parce que c'est lui qui peut le nommer : le §6
    // ouvre un script par `TARGET <machine>`, et un auteur qui a ecrit sa carte
    // n'a pas a redire sa machine sur la ligne de commande.
    script::Script scr;
    if (!scriptPath.empty()) {
        std::string text;
        if (!readFile(scriptPath, text)) {
            fprintf(stderr, "error: file not found: %s\n", scriptPath.c_str());
            return 2;
        }
        scr = script::parse(text, scriptPath);
        for (const asmb::Diagnostic &d : scr.errors)
            fprintf(stderr, "%s:%d: error: %s\n", d.file.c_str(), d.line, d.message.c_str());
        if (!scr.ok) return 1;
        if (scr.hasTarget && targetName.empty() && profilePath.empty())
            targetName = scr.target;
        else if (scr.hasTarget && !targetName.empty() && scr.target != targetName) {
            fprintf(stderr, "error: the script targets '%s' and --target says '%s'; keep one\n",
                    scr.target.c_str(), targetName.c_str());
            return 2;
        }
    }

    // Un nom livre ET un fichier : refuse. C'est la regle du §7 appliquee a la
    // ligne de commande — deux porteurs pour une meme chose, et personne ne
    // pourrait dire lequel a servi.
    if (!targetName.empty() && !profilePath.empty()) {
        fprintf(stderr, "error: --target and -P both name a profile; keep one\n");
        return 2;
    }
    bool hasProfile = false;
    profile::Profile prof;
    if (!targetName.empty() || !profilePath.empty()) {
        std::string text, from;
        if (!targetName.empty()) {
            text = profile::builtin(targetName);
            from = targetName;
            if (text.empty()) {
                fprintf(stderr, "error: no built-in profile named '%s' (built-in: %s)\n"
                                "       a profile is a data file: write one and pass it with -P\n",
                        targetName.c_str(), namesList().c_str());
                return 2;
            }
        } else {
            if (!readFile(profilePath, text)) {
                fprintf(stderr, "error: file not found: %s\n", profilePath.c_str());
                return 2;
            }
            from = profilePath;
        }
        prof = profile::parse(text, from);
        for (const asmb::Diagnostic &d : prof.errors)
            fprintf(stderr, "%s:%d: error: %s\n", d.file.c_str(), d.line, d.message.c_str());
        if (!prof.ok) return 1;
        hasProfile = true;
    }
    // Un script qui place sans profil est refuse DANS le linker, la ou la
    // raison se dit completement ; le CLI n'a pas a la dupliquer.
    (void)hasProfile;

    // Les SYMBOLES DE COMMUTATION du §12.3, calcules ICI, avant d'assembler.
    //
    // Ils ne dependent d'aucune adresse : un port, une valeur bornee aux bits de
    // son axe et un masque se calculent des que l'etat, son argument et sa banque
    // sont connus. Les passer a l'assembleur comme des CONSTANTES leur donne
    // l'arithmetique que le §12.2 emploie — `ld bc, __port_x + __val_x` devient
    // une somme de deux nombres — et l'usage sur un octet, qu'un EXTERN ne peut
    // pas leur donner puisqu'un EXTERN est une adresse.
    //
    // L'assembleur ne CONNAIT toujours aucune machine : il recoit des chiffres,
    // comme un compilateur C recoit ses `-D`. Et le linker les offre AUSSI a ses
    // EXTERN, par la meme fonction, pour l'unite qui a ete assemblee sans script.
    const asmb::Constants given = link::switchSymbols(scr, prof);

    if (!inputs.empty()) path = inputs.front();
    // Un `.fo` en ENTREE est un objet deja assemble : on le relit au lieu de
    // l'assembler. Un `.fo` en SORTIE demande l'inverse — assembler seul, et
    // s'arreter la. Deduit de l'extension, comme `.sna` : c'est le fichier qui
    // dit ce qu'il est, et l'auteur n'a pas un drapeau de plus a retenir.
    auto isFo = [](const std::string &p) {
        return p.size() >= 3 && p.substr(p.size() - 3) == ".fo";
    };
    if (path.empty()) { fprintf(stderr, "usage: fantams (file.asm | file.fo...) [-o out] [-s] [-E] [--strict] [--beautify] [--normalize] [--no-detach-labels] [--no-indent-blocks] [--base base.sna] [--sym[=out.sym]]\n"
                                     "  -o out.fo  : assembler SEUL et ecrire l'objet, sans linker\n"
                                     "  file.fo... : des objets deja assembles, a linker\n"
                                     "  --target N : profil de cible livre (au choix : %s)\n"
                                     "  -P f.prof  : un profil de cible a soi, par le meme chemin de code\n"
                                     "  -T f.ld    : le script de linkage — quelle section va ou\n"
                                     "  --dump-profile N : ecrire le profil livre N sur la sortie standard\n",
                                     namesList().c_str()); return 2; }
    // Le mode « la sortie est un source » : l'un ou l'autre des deux drapeaux suffit.
    const bool sourceOut = beautifyOnly || normalizeOnly;
    if (outPath.empty()) {
        size_t dot = path.find_last_of('.');
        std::string stem = (dot == std::string::npos ? path : path.substr(0, dot));
        outPath = stem + (sourceOut ? ".fmt.asm" : dumpOnly ? ".pp.asm" : ".bin");
    }

    // Le .sym decrit le BINAIRE, pas la source : il se derive de -o pour se poser
    // a cote de lui, la ou un emulateur ira le chercher.
    if (wantSym && symPath.empty()) {
        size_t dot = outPath.find_last_of('.');
        size_t slash = outPath.find_last_of('/');
        std::string stem = (dot == std::string::npos || (slash != std::string::npos && dot < slash))
                               ? outPath : outPath.substr(0, dot);
        symPath = stem + ".sym";
    }
    // Ni --beautify ni --normalize ne passent par l'assembleur : il n'y a pas de
    // table de symboles sans assemblage. -E, lui, assemble — il est autorise.
    if (wantSym && sourceOut) {
        fprintf(stderr, "error: --sym demande un assemblage ; %s ne passe pas par l'assembleur (il transforme du texte en texte)\n",
                beautifyOnly ? "--beautify" : "--normalize");
        return 2;
    }

    if (beautifyOnly && dumpOnly) {
        fprintf(stderr, "error: -E et --beautify demandent deux sorties differentes : la source deroulee, ou le source mis en forme\n");
        return 2;
    }
    if (normalizeOnly && dumpOnly) {
        fprintf(stderr, "error: -E canonise deja PUIS deroule les macros et les boucles ; --normalize canonise sans derouler. Deux sorties differentes.\n");
        return 2;
    }

    const bool wantFo = isFo(outPath);
    if (wantFo && (dumpOnly || sourceOut)) {
        fprintf(stderr, "error: un .fo est un objet assemble ; %s ne passe pas par l'assembleur\n",
                dumpOnly ? "-E" : (beautifyOnly ? "--beautify" : "--normalize"));
        return 2;
    }
    bool wantSna = outPath.size() >= 4 && outPath.substr(outPath.size() - 4) == ".sna";
    if (!basePath.empty() && (dumpOnly || sourceOut || !wantSna)) {
        fprintf(stderr, "error: --base ne s'applique qu'a une sortie .sna : %s\n", outPath.c_str());
        return 2;
    }

    // Aucun repli silencieux sur une base : un repli sur des zeros produirait un
    // .sna qui demarre et plante au premier appel firmware (ADR 0012).
    sna::Base base;
    bool hasBase = false;
    if (!basePath.empty()) {
        std::string raw;
        if (!readFile(basePath, raw)) {
            fprintf(stderr, "error: base introuvable : %s\n", basePath.c_str());
            return 2;
        }
        std::vector<uint8_t> bytes(raw.begin(), raw.end());
        std::string err;
        if (!sna::parseBase(bytes, base, err)) {
            fprintf(stderr, "error: %s : %s\n", basePath.c_str(), err.c_str());
            return 2;
        }
        hasBase = true;
    }

    // Les entrees `.fo` sont des objets DEJA assembles : on les relit, on ne les
    // reassemble pas. C'est tout l'objet de la compilation separee.
    std::vector<asmb::Object> objects;
    std::vector<std::string> sources;
    for (const std::string &in : inputs) {
        if (!isFo(in)) { sources.push_back(in); continue; }
        std::string raw;
        if (!readFile(in, raw)) { fprintf(stderr, "error: file not found: %s\n", in.c_str()); return 2; }
        asmb::Object obj;
        std::string err;
        if (!fo::read(raw, obj, err)) {
            fprintf(stderr, "error: %s: %s\n", in.c_str(), err.c_str());
            return 1;
        }
        obj.name = in;
        objects.push_back(std::move(obj));
    }
    if (sources.size() > 1) {
        fprintf(stderr, "error: un seul source .asm a la fois ; assemble-les separement en .fo puis linke-les\n");
        return 2;
    }
    if (!sources.empty() && !objects.empty() && !wantFo) {
        // Melanger un source et des objets marcherait, mais cacherait quel
        // fichier a ete reassemble : la compilation separee vaut par le fait
        // qu'on SAIT ce qui a ete refait.
        fprintf(stderr, "error: melange d'un source et d'objets ; assemble le source en .fo d'abord\n");
        return 2;
    }
    if (sources.empty() && objects.empty()) {
        fprintf(stderr, "error: rien a assembler ni a linker\n");
        return 2;
    }

    // Tout ce qui suit — preprocesseur, mise en forme, assemblage — ne concerne
    // qu'un SOURCE. Quand il n'y en a pas, les objets sont deja la et l'on tombe
    // directement dans le linkage.
    asmb::Object out;
    std::string content;
    if (!sources.empty()) {
    path = sources.front();
    if (!readFile(path, content)) { fprintf(stderr, "error: file not found: %s\n", path.c_str()); return 2; }

    // --beautify : la mise en forme rend le source de l'AUTEUR — ses macros, ses
    // includes et ses boucles restent ou ils sont, rien n'est deroule.
    //
    // Le preprocesseur tourne quand meme, mais on ne garde de lui que sa TABLE DE
    // MACROS : c'est la seule chose qui voie a l'interieur des `include`, et sans
    // elle la mise en forme ne peut pas distinguer « fill_screen » appele de
    // « fill_screen » declare. Ses erreurs sont ignorees — la mise en forme doit
    // tourner sur un source casse, c'est meme la qu'on la demande le plus.
    //
    // Temps PREPROCESSEUR : ce texte est celui que le preprocesseur va lire, ses
    // mots-cles y sont donc vivants. Au temps d'assemblage, « MEND » n'est pas
    // reserve et serait lu comme un label seul sur sa ligne — le beautify lui
    // ajouterait un deux-points et detruirait le source.
    if (sourceOut) {
        // L'ordre compte : --normalize change le nombre de lignes, le beautify met
        // en forme ce qui en resulte. L'inverse mettrait en forme des lignes que
        // la canonisation allait couper.
        std::string text = content;
        if (normalizeOnly) text = pp::normalize(text);
        if (beautifyOnly) {
            const pp::Result probe = pp::preprocess(content, path, readFile, /*strict=*/false);
            text = beautify::apply(text, kw::Phase::Preprocess, detachLabels, indentBlocks,
                                   probe.macroNames);
        }
        std::ofstream f(outPath, std::ios::binary);
        if (!f) { fprintf(stderr, "error: cannot write: %s\n", outPath.c_str()); return 2; }
        f.write(text.data(), (std::streamsize)text.size());
        fprintf(stderr, "%s: %s\n", outPath.c_str(),
                beautifyOnly && normalizeOnly ? "source canonise et mis en forme"
                : normalizeOnly ? "source canonise" : "source mis en forme");
        return 0;
    }

    // 1) preprocessor
    pp::Result pre = pp::preprocess(content, path, readFile, strict);
    for (auto &w : pre.warnings) fprintf(stderr, "%s:%d: warning: %s\n", w.file.c_str(), w.line, w.message.c_str());
    if (!pre.ok) {
        for (auto &e : pre.errors) fprintf(stderr, "%s:%d: error (preproc): %s\n", e.file.c_str(), e.line, e.message.c_str());
        return 1;
    }

    // -E : la source deroulee est le resultat demande, on s'arrete la. Elle sort
    // MISE EN FORME (ADR 0013), au temps d'ASSEMBLAGE : c'est le texte que
    // l'assembleur va lire, les mots-cles du preprocesseur n'y sont plus. Les
    // traiter comme reserves ferait indenter un label nomme « read » au lieu de
    // lui donner son deux-points.
    if (dumpOnly) {
        std::string text = beautify::apply(pre.dump(), kw::Phase::Assembly, detachLabels, indentBlocks);
        std::ofstream f(outPath, std::ios::binary);
        if (!f) { fprintf(stderr, "error: cannot write: %s\n", outPath.c_str()); return 2; }
        f.write(text.data(), (std::streamsize)text.size());
        // Le compte est celui du TEXTE ECRIT, pas celui de pre.lines : le
        // detachement des labels ajoute des lignes, et annoncer l'autre chiffre
        // ferait mentir le seul nombre que le lecteur peut verifier.
        size_t written = text.empty() ? 0 : 1;
        for (char c : text) if (c == '\n') ++written;
        if (!text.empty() && text.back() == '\n') --written;
        fprintf(stderr, "%s: unrolled source (%zu lines)\n", outPath.c_str(), written);
        // --sym exige d'assembler, on continue donc. Aucun binaire ne sera ecrit :
        // -E a pris `-o`, et l'ecraser detruirait la sortie demandee.
        if (!wantSym) return 0;
    }

    // 2) assembler (2 passes) on the flat text
    std::vector<asmb::SourceLine> lines;
    for (auto &l : pre.lines) lines.push_back({l.text, l.file, l.line, l.col0});
    out = asmb::assemble(lines, given);
    out.name = path;
    objects.push_back(out);
    // PRINT n'est ni une erreur ni un avertissement : c'est ce que la source a
    // demande d'afficher. Sur stderr comme le reste, pour que stdout reste libre
    // (l'option -E y ecrit la source deroulee).
    for (auto &p : out.prints) fprintf(stderr, "%s:%d: %s\n", p.file.c_str(), p.line, p.message.c_str());

    }   // fin du chemin « il y a un source »

    // `-o quelque-chose.fo` : ECRIRE UN OBJET, et ne rien linker. C'est ce que la
    // compilation separee demande — et c'est aussi ce qui rend l'aller-retour
    // verifiable, puisqu'un `.fo` relu se reecrit par le meme chemin.
    if (wantFo) {
        for (auto &w : out.warnings) fprintf(stderr, "%s:%d: warning: %s\n", w.file.c_str(), w.line, w.message.c_str());
        if (!out.ok) {
            for (auto &e : out.errors) fprintf(stderr, "%s:%d: error: %s\n", e.file.c_str(), e.line, e.message.c_str());
            return 1;
        }
        // Un objet est UNE unite de compilation : en fusionner plusieurs
        // demanderait de renumeroter sections, fragments et sites, et ce serait
        // un linkage partiel qui ne dit pas son nom.
        if (objects.size() != 1) {
            fprintf(stderr, "error: un .fo est UNE unite de compilation ; %zu objets ont ete donnes\n",
                    objects.size());
            return 2;
        }
        const std::string text = fo::write(objects.front());
        std::ofstream f(outPath, std::ios::binary);
        if (!f) { fprintf(stderr, "error: cannot write: %s\n", outPath.c_str()); return 2; }
        f.write(text.data(), (std::streamsize)text.size());
        fprintf(stderr, "%s: object (%zu fragments, %zu relocations, %zu symbols)\n",
                outPath.c_str(), objects.front().fragments.size(),
                objects.front().relocs.size(), objects.front().symbolTable.size());
        return 0;
    }

    // 3) linkage : l'assembleur a rendu un OBJET, le linker en fait une IMAGE.
    // C'est le seul chemin par lequel un octet sort d'ici, et c'est lui qui
    // decide des banques, des adresses et du recouvrement — le CLI n'en derive
    // plus aucune.
    const link::Image img = link::build(objects, scr, prof);
    for (auto &w : out.warnings) fprintf(stderr, "%s:%d: warning: %s\n", w.file.c_str(), w.line, w.message.c_str());
    for (auto &w : img.warnings) fprintf(stderr, "%s:%d: warning: %s\n", w.file.c_str(), w.line, w.message.c_str());
    // Le mou de chaque banque remplie : ni erreur ni avertissement, meme canal
    // que PRINT. Un mou n'est pas un defaut, et le crier en avertissement
    // apprendrait a ignorer les avertissements. Il sort AVANT le refus, et donc
    // aussi quand tout va bien : c'est le cas ou il sert.
    for (auto &e : img.prints) fprintf(stderr, "%s:%d: %s\n", e.file.c_str(), e.line, e.message.c_str());
    if (!out.ok || !img.ok) {
        for (auto &e : out.errors) fprintf(stderr, "%s:%d: error: %s\n", e.file.c_str(), e.line, e.message.c_str());
        for (auto &e : img.errors) fprintf(stderr, "%s:%d: error: %s\n", e.file.c_str(), e.line, e.message.c_str());
        return 1;
    }

    // La table des symboles sort ICI : apres le refus sur erreur d'assemblage — un
    // .sym partiel qu'un debogueur charge sans le savoir est pire que pas de .sym —
    // mais AVANT le refus des banques >= 8, ou l'assemblage a reussi et ou seul
    // l'export a plat echoue. C'est justement la que les adresses sont utiles.
    if (wantSym) {
        const std::string table = sym::format(img);
        std::ofstream f(symPath, std::ios::binary);
        if (!f) { fprintf(stderr, "error: cannot write: %s\n", symPath.c_str()); return 2; }
        f.write(table.data(), (std::streamsize)table.size());
        fprintf(stderr, "%s: %zu symboles\n", symPath.c_str(), img.symbolTable.size());
    }
    if (dumpOnly) return 0;   // -E --sym : les deux sorties demandees sont ecrites

    // Le dump est PLAT : 64 Ko s'il ne sort pas des banques 0..3, 128 Ko pour le
    // 6128 complet. Au-dela de la banque 7, aucun dump plat ne peut porter les
    // octets : il faudrait les chunks MEM du v3 (ADR 0006). On le dit en nommant
    // les banques, plutot que d'ecrire un fichier ampute qui aurait l'air correct.
    int dumpKo = 64;
    std::string tooHigh;
    for (int b : img.banksWritten) {
        if (b >= 8) tooHigh += (tooHigh.empty() ? "" : ", ") + std::to_string(b);
        else if (b >= 4) dumpKo = 128;
    }
    if (!tooHigh.empty()) {
        fprintf(stderr, "error: bank(s) %s written, beyond bank 7: a flat dump stops at 128K. "
                        "Assembling there works, exporting does not yet — it needs the chunked v3 "
                        "snapshot.\n", tooHigh.c_str());
        return 1;
    }

    // 4) write out : .sna -> snapshot ; otherwise raw binary
    bool asSna = wantSna;
    std::vector<uint8_t> data;
    if (asSna) {
        // Le backend de snapshot n'est pas touche a cet etage : il prend une
        // image plate et une coverage separee, et c'est le linker qui les lui
        // reconstitue. Sa signature est une dette reelle, mais c'est celle du
        // builder.
        const link::Flat flat = link::flatten(img);
        sna::Options o; o.pc = img.runAddress;
        data = sna::build(flat.bytes, o, hasBase ? &base : nullptr,
                          hasBase ? &flat.covered : nullptr, dumpKo);
    } else {
        // Le binaire brut est un intervalle contigu d'adresses logiques : il n'a
        // pas de place pour dire « et ces octets-la sont en banque 5 ».
        if (dumpKo > 64)
            fprintf(stderr, "warning: banks beyond the base 64K were written; a raw binary cannot "
                            "carry them — export a .sna to keep them\n");
        data = img.bin;
    }
    std::ofstream f(outPath, std::ios::binary);
    if (!f) { fprintf(stderr, "error: cannot write: %s\n", outPath.c_str()); return 2; }
    f.write((const char *)data.data(), (std::streamsize)data.size());
    if (asSna) {
        if (hasBase)
            fprintf(stderr, "base: %s (CPCType %d)\n", basePath.c_str(), (int)base.cpcType);
        else
            fprintf(stderr, "base: aucune (hors du code assemble, la memoire vaut zero)\n");
        fprintf(stderr, "%s: snapshot (%zu bytes), PC=0x%04X\n", outPath.c_str(), data.size(), img.runAddress);
    }
    else
        fprintf(stderr, "%s: %zu bytes @ 0x%04X\n", outPath.c_str(), data.size(), img.loadAddress);

    if (showSyms)
        for (auto &s : out.symbols)
            fprintf(stderr, "  %-20s = 0x%04llX\n", s.first.c_str(), (unsigned long long)(s.second & 0xFFFF));
    return 0;
}
