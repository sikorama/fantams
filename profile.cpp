// profile.cpp - Le profil de cible, analysé (voir profile.h)
//
// Le second consommateur du curseur de `lex`, et celui qui l'a fait exister.
// Sa grammaire est plus riche que celle du script sur un point : `SELECT` porte
// des EXPRESSIONS, parce qu'une valeur de commutation est un assemblage de
// champs de bits et non un nombre — `%11000000 | (PAGE << 3) | CODE`.
#include "profile.h"

#include "lex.h"

#include <cctype>
#include <cstdlib>

namespace profile {
namespace {

using lex::Tok;

Expr num(int64_t v) { Expr e; e.kind = Expr::Num; e.num = v; return e; }

struct Parser : lex::Cursor {
    Profile out;

    // La reprise après une faute : jusqu'à ce qui peut recommencer une
    // déclaration. Même dessin qu'au script, et même garantie de progrès — une
    // reprise qui retombe sur le jeton fautif boucle.
    void sync() {
        for (; !atEnd(); next()) {
            if (isPunct("}")) return;
            if (cur().kind != Tok::Name) continue;
            const std::string &w = cur().s;
            if (w == "TARGET" || w == "WINDOW" || w == "BANK" || w == "CONFIG" ||
                w == "SELECT" || w == "PAGING")
                return;
        }
    }

    bool known(const std::vector<Window> &v, const std::string &n) const {
        for (const Window &w : v) if (w.name == n) return true;
        return false;
    }
    bool known(const std::vector<Bank> &v, const std::string &n) const {
        for (const Bank &b : v) if (b.name == n) return true;
        return false;
    }
    Bank *bank(const std::string &n) {
        for (Bank &b : out.banks) if (b.name == n) return &b;
        return nullptr;
    }
    bool axisKnown(const std::string &n) const {
        for (const Axis &a : out.axes) if (a.name == n) return true;
        return false;
    }

    // --- Les expressions ----------------------------------------------------
    // Une escalade de précédence, dans l'ordre du C : ce sont les opérateurs
    // avec lesquels une valeur de commutation s'écrit, et leur ordre est celui
    // qu'un auteur de source Z80 a déjà dans les doigts.
    int precedence(const std::string &op) const {
        if (op == "|") return 1;
        if (op == "^") return 2;
        if (op == "&") return 3;
        if (op == "<<" || op == ">>") return 4;
        if (op == "+" || op == "-") return 5;
        if (op == "*" || op == "/") return 6;
        return 0;
    }

    bool primary(Expr &e) {
        if (cur().kind == Tok::Number) { e = num(cur().n); next(); return true; }
        if (cur().kind == Tok::Name) {
            e.kind = Expr::Name;
            e.name = cur().s;
            next();
            return true;
        }
        if (isPunct("(")) {
            next();
            if (!expr(e, 0)) return false;
            return want(")");
        }
        if (isPunct("~") || isPunct("-")) {
            Expr u;
            u.kind = Expr::Unary;
            u.op = cur().s;
            next();
            Expr a;
            if (!primary(a)) return false;
            u.args.push_back(std::move(a));
            e = std::move(u);
            return true;
        }
        err("expected a value, got " + got());
        return false;
    }

    bool expr(Expr &e, int minPrec) {
        if (!primary(e)) return false;
        for (;;) {
            if (cur().kind != Tok::Punct) return true;
            // Un `?` est le seul opérateur que ce langage n'a pas, et c'est un
            // refus nommé plutôt qu'une surprise. La forme qu'il servait —
            // « ce bit-ci de ce registre-là vaut ceci dans cet état » — disait
            // deux choses à la fois : un MASQUE, et une valeur par état. Les
            // deux sont maintenant explicites et séparées, et le nom du
            // registre a quitté la grammaire avec le ternaire.
            if (isPunct("?")) {
                err("a conditional is not part of a SELECT value: give the axis its "
                    "MASK, and let each state carry the value of its own bits "
                    "with [CODE ...]");
                return false;
            }
            const int p = precedence(cur().s);
            if (p == 0 || p < minPrec) return true;
            Expr b;
            b.kind = Expr::Binary;
            b.op = cur().s;
            next();
            Expr rhs;
            if (!expr(rhs, p + 1)) return false;
            b.args.push_back(std::move(e));
            b.args.push_back(std::move(rhs));
            e = std::move(b);
        }
    }

    // --- TARGET -------------------------------------------------------------
    void target() {
        const int line = cur().line;
        next();
        if (out.hasTarget) {
            err(line, "TARGET is declared twice: a profile describes one machine");
            sync();
            return;
        }
        std::string n;
        if (!wantName(n)) { sync(); return; }
        out.hasTarget = true;
        out.target = n;
    }

    // --- WINDOW nom [lo..hi] ------------------------------------------------
    void window() {
        const int line = cur().line;
        next();
        Window w;
        w.line = line;
        if (!wantName(w.name)) { sync(); return; }
        if (!want("[")) { sync(); return; }
        if (!wantNumber(w.lo)) { sync(); return; }
        if (!isPunct("..")) {
            err("a window is a RANGE: write '[0x4000..0x7FFF]'");
            sync();
            return;
        }
        next();
        if (!wantNumber(w.hi)) { sync(); return; }
        if (!want("]")) { sync(); return; }
        if (w.hi < w.lo) { err(line, "WINDOW '" + w.name + "': the range ends before it starts"); return; }
        // Deux fenêtres du même nom : refusé. En retenir une ferait dépendre la
        // carte de l'ordre des lignes, ce que personne n'a écrit.
        for (const Window &prev : out.windows)
            if (prev.name == w.name) {
                err(line, "WINDOW '" + w.name + "' is declared twice (first at line " +
                          std::to_string(prev.line) + ")");
                return;
            }
        out.windows.push_back(std::move(w));
    }

    // --- Un nom de banque, seul, en plage, ou paramétrique ------------------
    // `base1`, `base0..base3`, `rom_hi<n>`. La plage est un raccourci d'écriture
    // et rien de plus : elle déclare N banques, et non une banque de N × 16 K.
    bool bankNames(std::vector<std::string> &names, std::vector<std::string> &params) {
        for (;;) {
            std::string n;
            if (!wantName(n)) return false;
            if (isPunct("<")) {
                next();
                std::string p;
                if (!wantName(p)) return false;
                if (!want(">")) return false;
                names.push_back(n);
                params.push_back(p);
            } else if (isPunct("..")) {
                next();
                std::string m;
                if (!wantName(m)) return false;
                // `base0..base3` : même préfixe, deux nombres. Le refus de tout
                // le reste est délibéré — un intervalle qu'on ne sait pas
                // énumérer n'est pas un intervalle.
                size_t k = 0;
                while (k < n.size() && !std::isdigit((unsigned char)n[k])) ++k;
                const std::string prefix = n.substr(0, k);
                if (k >= n.size() || m.size() <= prefix.size() ||
                    m.compare(0, prefix.size(), prefix) != 0) {
                    err("'" + n + ".." + m + "' is not a range of banks: both names "
                        "must share a prefix and end with a number");
                    return false;
                }
                const long a = std::strtol(n.c_str() + k, nullptr, 10);
                const long b = std::strtol(m.c_str() + prefix.size(), nullptr, 10);
                if (b < a) { err("'" + n + ".." + m + "' counts backwards"); return false; }
                for (long v = a; v <= b; ++v) {
                    names.push_back(prefix + std::to_string(v));
                    params.push_back(std::string());
                }
            } else {
                names.push_back(n);
                params.push_back(std::string());
            }
            if (!isPunct(",")) return true;
            next();
        }
    }

    // --- BANK … SIZE … attributs -------------------------------------------
    void bankDecl() {
        const int line = cur().line;
        next();
        std::vector<std::string> names, params;
        if (!bankNames(names, params)) { sync(); return; }
        bool hasSize = false;
        int64_t size = 0, page = 0;
        bool ro = false, rw = false, video = false, contended = false, hasPage = false;
        for (;;) {
            if (isName("SIZE")) {
                next();
                if (!wantNumber(size)) { sync(); return; }
                hasSize = true;
            } else if (isName("PAGE")) {
                next();
                if (!wantNumber(page)) { sync(); return; }
                hasPage = true;
            } else if (isName("ro")) { ro = true; next(); }
            else if (isName("rw")) { rw = true; next(); }
            else if (isName("VIDEO")) { video = true; next(); }
            else if (isName("CONTENDED")) { contended = true; next(); }
            else break;
        }
        if (ro && rw) { err(line, "BANK: 'ro' and 'rw' cannot both be declared"); return; }
        for (size_t k = 0; k < names.size(); ++k) {
            Bank *prev = bank(names[k]);
            // Une ligne SANS taille AMENDE des banques déjà déclarées — c'est
            // ainsi qu'on ajoute `CONTENDED` à quatre d'entre elles sans répéter
            // leur taille. Une banque qu'aucune ligne ne dimensionne est refusée
            // à la fin : c'est là qu'on peut le savoir.
            if (prev) {
                if (hasSize && prev->hasSize) {
                    err(line, "BANK '" + names[k] + "' is declared twice with a size "
                              "(first at line " + std::to_string(prev->line) + ")");
                    continue;
                }
                if (hasSize) { prev->hasSize = true; prev->size = size; }
                if (ro) prev->readOnly = true;
                if (rw) prev->readOnly = false;
                if (video) prev->video = true;
                if (contended) prev->contended = true;
                if (hasPage) { prev->hasPage = true; prev->page = page; }
                continue;
            }
            Bank b;
            b.name = names[k];
            b.hasSize = hasSize;
            b.size = size;
            b.readOnly = ro;
            b.video = video;
            b.contended = contended;
            b.hasPage = hasPage;
            b.page = page;
            b.line = line;
            if (!params[k].empty()) { b.hasPage = true; b.page = 0; }
            out.banks.push_back(std::move(b));
        }
    }

    // --- CONFIG SET axe [OVER axe] { états } --------------------------------
    void configSet() {
        const int line = cur().line;
        next();
        if (!isName("SET")) {
            err("expected 'CONFIG SET <axis>', got " + got() +
                " — a profile declares axes; a script names their states");
            sync();
            return;
        }
        next();
        Axis a;
        a.line = line;
        if (!wantName(a.name)) { sync(); return; }
        if (axisKnown(a.name)) {
            err(line, "CONFIG SET '" + a.name + "' is declared twice");
            skipBlock();
            return;
        }
        if (isName("OVER")) {
            next();
            if (!wantName(a.over)) { sync(); return; }
            if (!axisKnown(a.over)) {
                err(line, "CONFIG SET '" + a.name + "' is declared OVER '" + a.over +
                          "', which is not an axis declared before it");
                skipBlock();
                return;
            }
        }
        if (!want("{")) { sync(); return; }
        while (!atEnd() && !isPunct("}")) {
            const size_t before = i;
            if (isName("MIRROR")) {
                err("MIRROR is recognized at stage C2: it is a third kind of "
                    "placement, and it goes with the continuity check it exists to "
                    "satisfy");
                skipBlock();
            } else if (!state(a)) {
                skipBlock();
            }
            if (i == before) next();
        }
        if (!want("}")) return;
        out.axes.push_back(std::move(a));
    }

    // --- un état : nom[<param>] [[CODE expr]] { fenêtre banque … } ----------
    bool state(Axis &a) {
        State s;
        s.line = cur().line;
        if (!wantName(s.name)) return false;
        for (const State &prev : a.states)
            if (prev.name == s.name) {
                err(s.line, "CONFIG SET '" + a.name + "': state '" + s.name +
                            "' is declared twice");
                return false;
            }
        if (isPunct("<")) {
            next();
            if (!wantName(s.param)) return false;
            if (!want(">")) return false;
            s.hasParam = true;
        }
        if (isPunct("[")) {
            next();
            if (!isName("CODE")) {
                err("expected CODE, got " + got() +
                    " — a state carries the value of its own bits, and nothing else");
                return false;
            }
            next();
            if (!expr(s.code, 0)) return false;
            s.hasCode = true;
            if (!want("]")) return false;
        }
        if (!want("{")) return false;
        while (!atEnd() && !isPunct("}")) {
            Slot sl;
            if (!wantName(sl.window)) return false;
            if (!known(out.windows, sl.window)) {
                err("CONFIG SET '" + a.name + "', state '" + s.name + "': '" +
                    sl.window + "' is not a declared WINDOW");
                return false;
            }
            if (!wantName(sl.bank)) return false;
            if (isPunct("<")) {
                next();
                if (cur().kind == Tok::Number) {
                    sl.literal = true;
                    sl.value = cur().n;
                    next();
                } else if (!wantName(sl.param)) return false;
                if (!want(">")) return false;
                sl.hasParam = true;
            }
            // Une banque paramétrique est déclarée sous son nom nu — `ext<b>`
            // renvoie à `BANK ext0..ext3` —, donc l'existence se vérifie sur le
            // nom résolu quand il est littéral, et sur le préfixe sinon.
            if (!sl.hasParam || sl.literal) {
                const std::string full = sl.literal ? sl.bank + std::to_string(sl.value) : sl.bank;
                if (!known(out.banks, full)) {
                    err("CONFIG SET '" + a.name + "', state '" + s.name + "': '" + full +
                        "' is not a declared BANK");
                    return false;
                }
            }
            s.slots.push_back(std::move(sl));
        }
        if (!want("}")) return false;
        a.states.push_back(std::move(s));
        return true;
    }

    // --- SELECT axe[..axe] = OUT|POKE port[, MASK m], valeur ---------------
    void selectStmt() {
        Select sel;
        sel.line = cur().line;
        next();
        std::string first;
        if (!wantName(first)) { sync(); return; }
        sel.axes.push_back(first);
        if (isPunct("..")) {
            next();
            std::string last;
            if (!wantName(last)) { sync(); return; }
            sel.axes.push_back(last);
        }
        // Les contraintes vérifiables — pile hors d'une plage, verrou, séquence —
        // sont RECONNUES et refusées en nommant l'étage qui les vérifie. Les
        // ignorer ferait produire un binaire faux en silence à un profil écrit
        // pour C2, ce qui est la faute même que ces contraintes attrapent.
        if (isName("STACK") || isName("CLOBBERS")) {
            err(cur().s + " is recognized at stage C2, which verifies switching "
                          "constraints; this stage only computes the numbers");
            sync();
            return;
        }
        if (!want("=")) { sync(); return; }
        while (isName("OUT") || isName("POKE")) {
            Write w;
            w.line = cur().line;
            w.kind = isName("OUT") ? Write::Out : Write::Poke;
            next();
            if (!expr(w.port, 0)) { sync(); return; }
            if (!want(",")) { sync(); return; }
            if (isName("MASK")) {
                next();
                if (!expr(w.mask, 0)) { sync(); return; }
                w.hasMask = true;
                if (!want(",")) { sync(); return; }
            }
            if (!expr(w.value, 0)) { sync(); return; }
            sel.writes.push_back(std::move(w));
        }
        if (sel.writes.empty()) {
            err(sel.line, "SELECT: expected OUT or POKE, got " + got() +
                          " — how an axis is reached is a sequence of writes");
            sync();
            return;
        }
        out.selects.push_back(std::move(sel));
    }

    void run() {
        while (!atEnd()) {
            const size_t before = i;
            if (isName("TARGET")) { target(); }
            else if (isName("WINDOW")) { window(); }
            else if (isName("BANK")) { bankDecl(); }
            else if (isName("CONFIG")) { configSet(); }
            else if (isName("SELECT")) { selectStmt(); }
            else if (isName("PAGING")) {
                // `PAGING LOCKS`, `PAGING WRITE_ONLY` : l'irréversibilité et le
                // port en écriture seule sont des contraintes, donc C2.
                err("PAGING declares switching constraints, which are recognized at "
                    "stage C2; this stage only computes the numbers");
                sync();
            }
            else if (isName("SHADOWS") || isName("ALWAYS")) {
                // Ce ne sont pas des mots du langage, et c'est une décision : deux
                // configurations qui n'accordent pas la même banque à une fenêtre
                // le DISENT par leur seule existence. C'était la principale source
                // d'erreur de saisie d'un profil (§13.1).
                err(cur().s + " is not a word of the profile language: it is "
                              "CALCULATED from the configurations — two states that "
                              "give a window different banks say it by existing");
                sync();
            }
            else {
                err("unknown keyword " + got() +
                    " (a profile has TARGET, WINDOW, BANK, CONFIG SET and SELECT)");
                sync();
            }
            if (i == before) next();
        }
        check();
    }

    // Ce qui ne se sait qu'à la fin.
    void check() {
        for (const Bank &b : out.banks)
            if (!b.hasSize)
                err(b.line, "BANK '" + b.name + "' has no SIZE: a bank's size is "
                            "declared, never implicit — a wired 16K would make "
                            "another machine indescribable");
        for (const Select &s : out.selects)
            for (const std::string &ax : s.axes)
                if (!axisKnown(ax))
                    err(s.line, "SELECT '" + ax + "' names no declared axis");
        for (const Axis &a : out.axes) {
            bool reached = false;
            for (const Select &s : out.selects)
                for (const std::string &ax : s.axes)
                    if (ax == a.name) reached = true;
            if (!reached)
                err(a.line, "CONFIG SET '" + a.name + "' has no SELECT: an axis "
                            "nothing can reach places sections the machine will "
                            "never show");
        }
    }
};

} // namespace

Profile parse(const std::string &text, const std::string &file) {
    Parser p;
    p.file = file;
    std::string err;
    int line = 0;
    if (!lex::tokenize(text, p.t, err, line)) {
        Profile bad;
        bad.ok = false;
        bad.errors.push_back({file, line, err});
        return bad;
    }
    p.run();
    p.out.ok = p.ok;
    p.out.errors = std::move(p.errors);
    return p.out;
}

} // namespace profile
