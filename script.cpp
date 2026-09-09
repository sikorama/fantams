// script.cpp - Le script de linkage, analysé (voir script.h)
//
// Une descente récursive sur le curseur partagé de `lex`. Le découpeur de jetons
// et la forme à blocs vivaient ici jusqu'à ce que le profil de cible leur donne
// un second consommateur ; ils sont maintenant dans `lex.h`, et l'extraction n'a
// changé aucun diagnostic de cette suite.
#include "script.h"

#include "lex.h"

#include <cctype>
#include <cstdlib>

namespace script {
namespace {

using lex::Tok;

// --- La descente ------------------------------------------------------------
struct Parser : lex::Cursor {
    Script out;

    // La reprise après une faute : on avance jusqu'à ce qui peut recommencer une
    // instruction, ou jusqu'à une accolade fermante. Sans elle, une virgule
    // oubliée produirait une cascade de diagnostics dont seul le premier est
    // vrai.
    void sync() {
        for (; !atEnd(); next()) {
            if (isPunct("}")) return;
            if (cur().kind != Tok::Name) continue;
            const std::string &w = cur().s;
            if (w == "CONFIG" || w == "SECTION" || w == "MIRROR" || w == "MEMORY_MAP" ||
                w == "OUTPUT_FORMAT" || w == "TARGET" || w == "ENTRY_POINT" ||
                w == "STACK" || w == "INT_VECTOR" || w == "CRO_ROM_NUMBER")
                return;
            if (w.size() >= 2 && w[0] == 'w' && std::isdigit((unsigned char)w[1])) return;
        }
    }


    // --- TARGET <machine> [+ <extension>] -----------------------------------
    void target() {
        const int line = cur().line;
        next();
        if (out.hasTarget) {
            err(line, "TARGET is declared twice: a script targets one machine");
            sync();
            return;
        }
        std::string name;
        if (!wantName(name)) { sync(); return; }
        out.hasTarget = true;
        out.target = name;
        while (isPunct("+")) {
            next();
            std::string ext;
            if (!wantName(ext)) { sync(); return; }
            out.extensions.push_back(ext);
        }
    }

    // --- Une référence de configuration : [axe.]état[<n>] -------------------
    bool configRef(ConfigRef &c) {
        std::string first;
        if (!wantName(first)) return false;
        if (isPunct(".")) {
            next();
            c.axis = first;
            if (!wantName(c.state)) return false;
        } else {
            c.state = first;
        }
        if (isPunct("<")) {
            next();
            if (!wantNumber(c.arg)) return false;
            c.hasArg = true;
            if (!want(">")) return false;
        }
        return true;
    }

    // --- w<n> [OFFSET x, SIZE y] { SECTION a  SECTION b } -------------------
    bool placement(Placement &p) {
        p.file = file;
        p.line = cur().line;
        // Un NOM, et le profil dira ce que cette fenêtre couvre. Reconnaître
        // ici un `w<chiffres>` aurait câblé UNE grille dans le langage : sur
        // d'autres machines les fenêtres portent d'autres noms, et deux grilles
        // superposées y sont actives en même temps (§13.1).
        //
        // `SECTION` est un nom recevable pour le découpeur, et c'est la faute la
        // plus probable à cet endroit : la nommer vaut mieux que de laisser
        // l'analyseur se plaindre d'une accolade manquante deux jetons plus loin.
        if (isName("SECTION")) {
            err("a SECTION goes inside a window: write 'w1 { SECTION main }' — it is "
                "the configuration that says which bank appears in which window, and "
                "the window that gives the section its ORG");
            return false;
        }
        if (!wantName(p.window)) return false;
        if (isPunct("[")) {
            next();
            // Les deux clés dans l'ordre du §6, et l'ordre est imposé : un
            // `SIZE` avant son `OFFSET` se lirait aussi bien, et deux écritures
            // pour un même placement rendraient un script moins comparable à un
            // autre.
            if (!isName("OFFSET")) { err("expected OFFSET, got " + got()); return false; }
            next();
            if (!wantNumber(p.offset)) return false;
            if (!want(",")) return false;
            if (!isName("SIZE")) { err("expected SIZE, got " + got()); return false; }
            next();
            if (!wantNumber(p.size)) return false;
            if (!want("]")) return false;
            p.hasRange = true;
        }
        if (!want("{")) return false;
        while (!atEnd() && !isPunct("}")) {
            if (isName("SECTION")) {
                next();
                std::string name;
                if (!wantName(name)) return false;
                p.sections.push_back(name);
                continue;
            }
            if (isName("COMPRESS")) {
                // RECONNU, et refusé. La compression change la taille, donc elle
                // appartient au placement (§8) — mais l'algorithme n'est pas de
                // cet étage, et un script à moitié appliqué produirait un binaire
                // faux sans un mot.
                err("COMPRESS is not applied at this stage: declare an envelope "
                    "instead — 'SECTION name, \"ro\", <max>' gives every following "
                    "address a known value, and the linker checks that the "
                    "compressed block fits");
                return false;
            }
            err("inside a window, expected SECTION, got " + got());
            return false;
        }
        return want("}");
    }

    // --- CONFIG linear { … } ------------------------------------------------
    bool configBlock(ConfigBlock &b) {
        b.file = file;
        b.line = cur().line;
        next();
        if (!configRef(b.config)) return false;
        // `CONFIG rom_upper.on, ROM 15 {` — les qualificatifs qui suivent la
        // virgule. Un nom, et une valeur s'il en porte.
        while (isPunct(",")) {
            next();
            Qualifier q;
            if (!wantName(q.name)) return false;
            if (cur().kind == Tok::Number) { q.hasValue = true; q.value = cur().n; next(); }
            b.qualifiers.push_back(std::move(q));
        }
        if (!want("{")) return false;
        while (!atEnd() && !isPunct("}")) {
            if (isName("MIRROR")) {
                // Le miroir est un TROISIÈME genre de placement (§13.3) : N copies
                // au même offset dans N banques. Il va avec la vérification de
                // continuité qu'il sert, donc avec l'étage C2.
                err("MIRROR is recognized at stage C2, which verifies the "
                    "continuity it exists to satisfy");
                return false;
            }
            Placement p;
            if (!placement(p)) return false;
            b.placements.push_back(std::move(p));
        }
        return want("}");
    }

    void memoryMap() {
        next();
        if (!want("{")) { sync(); return; }
        while (!atEnd() && !isPunct("}")) {
            const size_t before = i;
            if (isName("CONFIG")) {
                ConfigBlock b;
                if (configBlock(b)) out.map.push_back(std::move(b));
                else skipBlock();
            } else {
                err("inside MEMORY_MAP, expected CONFIG, got " + got());
                skipBlock();
            }
            // La garantie de progrès, et elle n'est pas décorative : sans elle,
            // une reprise qui retombe sur le jeton fautif boucle sans fin en
            // empilant le même diagnostic.
            if (i == before) next();
        }
        want("}");
    }

    // --- OUTPUT_FORMAT { CLÉ = valeur … } -----------------------------------
    void outputFormat() {
        next();
        if (!want("{")) { sync(); return; }
        while (!atEnd() && !isPunct("}")) {
            const size_t before = i;
            if (cur().kind != Tok::Name) {
                err("inside OUTPUT_FORMAT, expected a key, got " + got());
                sync();
                if (i == before) next();
                continue;
            }
            const std::string key = cur().s;
            const int line = cur().line;
            next();
            if (!want("=")) { sync(); continue; }
            if (key == "CONTAINER") {
                if (cur().kind != Tok::Text) {
                    err("CONTAINER: expected a string, got " + got());
                    sync();
                    continue;
                }
                out.output.hasContainer = true;
                out.output.container = cur().s;
                next();
            } else if (key == "TARGET") {
                // `TARGET` nomme LA MACHINE, au premier niveau du script. Le §6
                // l'employait aussi pour le conteneur ; ce bloc a circulé, et un
                // refus muet le laisserait recopier. Il est donc nommé, avec sa
                // graphie de remplacement.
                err(line, "TARGET names the machine, at the top level of a script: "
                          "write 'CONTAINER = ...' for the output container");
                sync();
            } else if (key == "ENTRY_POINT") {
                if (!wantNumber(out.output.entry)) { sync(); continue; }
                out.output.hasEntry = true;
            } else if (key == "INT_VECTOR") {
                if (!wantNumber(out.output.intVector)) { sync(); continue; }
                out.output.hasIntVector = true;
            } else if (key == "CRO_ROM_NUMBER") {
                if (!wantNumber(out.output.romNumber)) { sync(); continue; }
                out.output.hasRomNumber = true;
            } else if (key == "STACK") {
                // Une PLAGE, et le refus d'une adresse seule est le fond de la
                // décision : `SP` bouge, et une adresse unique ne dit rien de
                // vrai d'un programme qui empile (§6).
                //
                // Le refus est le MÊME dans les deux formes fautives — `= 0x3FFF`
                // et `= [0x3FFF]` —, parce que c'est la même faute. Laisser le
                // premier cas tomber sur un « expected '[' » dirait la syntaxe
                // sans dire la raison, à l'endroit précis où la raison est tout.
                auto notARange = [&] {
                    err(line, "STACK is a range, not an address: write "
                              "'[0x3F00..0x3FFF]' — SP moves, and a single address says "
                              "nothing true of a program that pushes");
                    sync();
                };
                if (!isPunct("[")) { notARange(); continue; }
                next();
                if (!wantNumber(out.output.stackLo)) { sync(); continue; }
                if (!isPunct("..")) { notARange(); continue; }
                next();
                if (!wantNumber(out.output.stackHi)) { sync(); continue; }
                if (!want("]")) { sync(); continue; }
                if (out.output.stackHi < out.output.stackLo)
                    err(line, "STACK: the range ends before it starts");
                out.output.hasStack = true;
            } else {
                err(line, "unknown OUTPUT_FORMAT key '" + key +
                          "' (CONTAINER, ENTRY_POINT, STACK, INT_VECTOR, CRO_ROM_NUMBER)");
                sync();
            }
        }
        want("}");
    }

    void run() {
        while (!atEnd()) {
            if (isName("TARGET")) { target(); continue; }
            if (isName("MEMORY_MAP")) { memoryMap(); continue; }
            if (isName("OUTPUT_FORMAT")) { outputFormat(); continue; }
            // Un mot inconnu est une ERREUR, jamais un silence. Le refus nomme
            // les trois blocs plutôt que de laisser chercher.
            err("unknown keyword " + got() +
                " (a script has TARGET, MEMORY_MAP and OUTPUT_FORMAT)");
            const size_t before = i;
            sync();
            if (i == before) next();   // sync() n'a rien avancé : ne pas boucler
        }
    }
};

} // namespace

Script parse(const std::string &text, const std::string &file) {
    Parser p;
    p.file = file;
    std::string err;
    int line = 0;
    if (!lex::tokenize(text, p.t, err, line)) {
        Script s;
        s.ok = false;
        s.errors.push_back({file, line, err});
        return s;
    }
    p.run();
    p.out.ok = p.ok;
    p.out.errors = std::move(p.errors);
    return p.out;
}

bool parseConfigRef(const std::string &text, ConfigRef &out, std::string &error) {
    Parser p;
    p.file.clear();
    std::string err;
    int line = 0;
    if (!lex::tokenize(text, p.t, err, line)) { error = err; return false; }
    if (!p.configRef(out)) {
        error = "'" + text + "' is not a configuration name (write 'state', "
                "'state<n>' or 'axis.state<n>')";
        return false;
    }
    // RIEN NE DOIT RESTER. Un reliquat voudrait dire qu'on a lu la moitié d'un
    // nom et accepté l'autre en silence, ce qui est la façon dont un placement
    // devient faux sans un mot.
    if (!p.atEnd()) {
        error = "'" + text + "' has trailing characters after the configuration name";
        return false;
    }
    return true;
}

} // namespace script
