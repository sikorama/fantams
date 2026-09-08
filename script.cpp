// script.cpp - Le script de linkage, analysé (voir script.h)
//
// Un découpeur de jetons, puis une descente récursive. Le découpeur est LOCAL à
// ce fichier, et c'est délibéré : le profil de cible emploiera la même grammaire
// à blocs, et c'est à ce moment-là — quand il aura deux consommateurs — qu'il
// sera extrait. L'extraire maintenant serait une couture hypothétique, celle que
// `coutures-de-la-chaine.md` §4 apprend à ne pas fabriquer.
#include "script.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace script {
namespace {

// --- Les jetons -------------------------------------------------------------
// Une seule subtilité dans ce découpage : `..` est un jeton, et non deux points.
// Sans quoi `[0x3F00..0x3FFF]` et `[ext0..ext3]` demanderaient au parseur de
// deviner, et `rom_upper.on` cesserait d'être lisible.
struct Tok {
    enum Kind { End, Name, Number, Text, Punct } kind = End;
    std::string s;        // le texte, pour Name / Text / Punct
    int64_t n = 0;        // la valeur, pour Number
    int line = 0;
};

bool nameStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
bool nameChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

// Les quatre notations de nombre que ce projet emploie déjà : `0x`, `&` et `#`
// pour l'hexadécimal, `%` pour le binaire, et le décimal nu. En refuser une
// serait demander à l'auteur d'un `.asm` d'écrire ses adresses autrement dans
// son script que dans sa source.
bool number(const std::string &t, size_t &i, Tok &out, std::string &err) {
    int base = 10;
    size_t a = i;
    if (t[i] == '&' || t[i] == '#') { base = 16; a = ++i; }
    else if (t[i] == '%') { base = 2; a = ++i; }
    else if (t[i] == '0' && i + 1 < t.size() && (t[i + 1] == 'x' || t[i + 1] == 'X')) {
        base = 16; i += 2; a = i;
    }
    while (i < t.size() && std::isalnum((unsigned char)t[i])) ++i;
    if (i == a) { err = "a number was expected"; return false; }
    const std::string digits = t.substr(a, i - a);
    char *end = nullptr;
    const long long v = std::strtoll(digits.c_str(), &end, base);
    if (!end || *end != '\0') { err = "'" + digits + "' is not a number"; return false; }
    out.kind = Tok::Number;
    out.n = (int64_t)v;
    out.s = digits;
    return true;
}

// Un `//` ou un `;` hors guillemets ouvre un commentaire — les deux, parce que
// le §6 écrit ses exemples avec `//` et qu'un source fantams commente avec `;`.
bool tokenize(const std::string &text, std::vector<Tok> &out, std::string &err, int &errLine) {
    int line = 1;
    for (size_t i = 0; i < text.size();) {
        const char c = text[i];
        if (c == '\n') { ++line; ++i; continue; }
        if (std::isspace((unsigned char)c)) { ++i; continue; }
        if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n') ++i;
            continue;
        }
        if (c == ';') {
            while (i < text.size() && text[i] != '\n') ++i;
            continue;
        }
        Tok t;
        t.line = line;
        if (c == '"') {
            ++i;
            for (;;) {
                if (i >= text.size() || text[i] == '\n') {
                    err = "unterminated string"; errLine = line; return false;
                }
                if (text[i] == '"') { ++i; break; }
                t.s += text[i++];
            }
            t.kind = Tok::Text;
            out.push_back(std::move(t));
            continue;
        }
        if (nameStart(c)) {
            size_t a = i;
            while (i < text.size() && nameChar(text[i])) ++i;
            t.kind = Tok::Name;
            t.s = text.substr(a, i - a);
            out.push_back(std::move(t));
            continue;
        }
        if (std::isdigit((unsigned char)c) || c == '&' || c == '#' || c == '%') {
            if (!number(text, i, t, err)) { errLine = line; return false; }
            out.push_back(std::move(t));
            continue;
        }
        if (c == '.' && i + 1 < text.size() && text[i + 1] == '.') {
            t.kind = Tok::Punct; t.s = ".."; i += 2;
            out.push_back(std::move(t));
            continue;
        }
        if (std::string("{}[]<>,=.+").find(c) != std::string::npos) {
            t.kind = Tok::Punct; t.s = std::string(1, c); ++i;
            out.push_back(std::move(t));
            continue;
        }
        err = std::string("unexpected character '") + c + "'";
        errLine = line;
        return false;
    }
    Tok end;
    end.line = line;
    out.push_back(end);
    return true;
}

// --- La descente ------------------------------------------------------------
struct Parser {
    std::vector<Tok> t;
    size_t i = 0;
    std::string file;
    Script out;

    const Tok &cur() const { return t[i]; }
    bool atEnd() const { return t[i].kind == Tok::End; }
    bool isName(const char *w) const { return cur().kind == Tok::Name && cur().s == w; }
    bool isPunct(const char *w) const { return cur().kind == Tok::Punct && cur().s == w; }
    void next() { if (!atEnd()) ++i; }

    void err(const std::string &msg) { err(cur().line, msg); }
    void err(int line, const std::string &msg) {
        out.ok = false;
        out.errors.push_back({file, line, msg});
    }

    // Ce que le jeton courant est, dit comme un diagnostic doit le dire : citer
    // « fin de fichier » plutôt que rien du tout est ce qui distingue une
    // accolade oubliée d'une faute de frappe.
    std::string got() const {
        switch (cur().kind) {
            case Tok::End:    return "end of file";
            case Tok::Number: return "'" + cur().s + "'";
            case Tok::Text:   return "a string";
            default:          return "'" + cur().s + "'";
        }
    }

    bool want(const char *p) {
        if (isPunct(p)) { next(); return true; }
        err(std::string("expected '") + p + "', got " + got());
        return false;
    }
    bool wantNumber(int64_t &v) {
        if (cur().kind == Tok::Number) { v = cur().n; next(); return true; }
        err("expected a number, got " + got());
        return false;
    }
    bool wantName(std::string &v) {
        if (cur().kind == Tok::Name) { v = cur().s; next(); return true; }
        err("expected a name, got " + got());
        return false;
    }

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

    // Sauter jusqu'à la fin du bloc courant, accolades comptées. Après une faute
    // DANS un bloc, reprendre à l'instruction suivante du même bloc produirait
    // une cascade de diagnostics dont seul le premier est vrai — et, si la
    // reprise retombe sur le jeton fautif, une boucle.
    void skipBlock() {
        int depth = 0;
        for (; !atEnd(); next()) {
            if (isPunct("{")) ++depth;
            else if (isPunct("}") && --depth <= 0) { next(); return; }
        }
    }

    // `w<n>` — le nom d'une fenêtre. Rendu comme un entier, parce que c'est le
    // profil qui dira ce que cette fenêtre couvre.
    bool windowIndex(int &n) {
        const std::string &w = cur().s;
        if (cur().kind != Tok::Name || w.size() < 2 || w[0] != 'w') return false;
        for (size_t k = 1; k < w.size(); ++k)
            if (!std::isdigit((unsigned char)w[k])) return false;
        n = std::atoi(w.c_str() + 1);
        next();
        return true;
    }

    // --- TARGET cpc6128 + RAM128 --------------------------------------------
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
        if (!windowIndex(p.window)) {
            err("expected a window name such as 'w1', got " + got());
            return false;
        }
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
            // `TARGET` désigne ici le CONTENEUR, et au premier niveau la MACHINE.
            // Le §6 emploie le même mot pour les deux ; l'analyseur les distingue
            // par leur place, et le nom est à revoir en C1.10.
            if (key == "TARGET") {
                if (cur().kind != Tok::Text) { err("TARGET: expected a string, got " + got()); sync(); continue; }
                out.output.hasFormat = true;
                out.output.format = cur().s;
                next();
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
                          "' (TARGET, ENTRY_POINT, STACK, INT_VECTOR, CRO_ROM_NUMBER)");
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
    if (!tokenize(text, p.t, err, line)) {
        Script s;
        s.ok = false;
        s.errors.push_back({file, line, err});
        return s;
    }
    p.run();
    return p.out;
}

} // namespace script
