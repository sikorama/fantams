// expr.cpp - Évaluateur d'expressions (voir expr.h)
//
// Calcul interne en double (comme l'assembleur de référence) : permet les littéraux flottants et les
// fonctions (sin/cos/abs/hi/lo) sans perte de précision intermédiaire — seule la
// valeur FINALE est convertie en entier (arrondi "half up", cf. toInt()). Les
// opérateurs bit à bit (| ^ & << >> ~ %) convertissent chaque opérande en entier
// avant de calculer (ils n'ont pas de sens en flottant), puis reconvertissent le
// résultat en double pour rester dans le même arbre d'évaluation.
//
// Chaque niveau de précédence rend une `Value` et non un `double` : une adresse
// dont la section sera placée au linkage n'est pas un nombre, et ce qu'on a le
// droit d'en calculer se décide OPÉRATEUR PAR OPÉRATEUR. Deux seulement la
// laissent relocalisable — `+` et `-` — plus `high()` et `low()` qui en
// prélèvent un octet ; tout le reste passe par `known()`, qui refuse. Le refus
// est le comportement par défaut, et c'est voulu : un opérateur ajouté demain
// sera refusé sur une adresse tant que personne n'aura écrit ce qu'il en fait.
#include "expr.h"

#include "keywords.h"

#include <cctype>
#include <cmath>
#include <set>
#include <stdexcept>

namespace expr {
namespace {

struct EvalError { std::string msg; };

// Arrondi "half up" (comme l'assembleur de référence : 3.5 -> 4, -3.5 -> -3 — cf. arrondi vers +infini,
// pas arrondi au plus proche pair ni troncature vers zéro). Vérifié empiriquement
// contre l'assembleur de référence (db 7/2 -> 4, db -7/2 -> -3).
int64_t toInt(double v) { return (int64_t)std::floor(v + 0.5); }

std::string lower(const std::string &n) {
    std::string o = n;
    for (char &c : o) c = (char)std::tolower((unsigned char)c);
    return o;
}

// Une valeur absolue, fabriquée depuis un nombre.
Value num(double d) { Value v; v.real = d; return v; }


struct Parser {
    const std::string &s;
    size_t i = 0;
    const Resolver &resolver;

    Parser(const std::string &str, const Resolver &r) : s(str), resolver(r) {}

    void skip() { while (i < s.size() && std::isspace((unsigned char)s[i])) ++i; }
    bool eof() { skip(); return i >= s.size(); }
    char peek() { skip(); return i < s.size() ? s[i] : 0; }
    bool eat(const char *op) {
        skip();
        size_t n = 0; while (op[n]) ++n;
        if (i + n <= s.size() && s.compare(i, n, op) == 0) { i += n; return true; }
        return false;
    }
    // Opérateur textuel (and/or/xor/not/mod/shl/shr/div) : casse indifférente, et
    // doit être suivi d'un caractère non-identifiant — sinon "android" serait lu
    // comme "and" suivi de "roid". Ces mots sont aussi des mnémoniques Z80, mais
    // sans conflit possible : parser.cpp sépare le mnémonique des opérandes avant
    // qu'expr ne voie quoi que ce soit ("ld a, b and 3" livre bien "b and 3").
    bool eatWord(const char *w) {
        skip();
        size_t n = 0; while (w[n]) ++n;
        if (i + n > s.size()) return false;
        for (size_t k = 0; k < n; ++k)
            if (std::tolower((unsigned char)s[i + k]) != std::tolower((unsigned char)w[k])) return false;
        if (i + n < s.size()) {
            char nx = s[i + n];
            if (std::isalnum((unsigned char)nx) || nx == '_' || nx == '.' || nx == '@') return false;
        }
        i += n;
        return true;
    }

    [[noreturn]] void fail(const std::string &m) { throw EvalError{m}; }

    // Le refus d'un opérateur sur une adresse qu'on ne connaît pas encore.
    // `what` se cite tel quel : « '*' », « 'sin()' ». Deux opérateurs reçoivent
    // le nom de leur remplaçant, parce que ce sont les deux idiomes que tout le
    // monde écrit : `label >> 8` et `label & 255`. Les RECONNAÎTRE comme des
    // motifs serait un piège — `label >> 9` leur ressemble sans en être, et
    // l'auteur à qui l'un échoue ne devinerait pas ce qui distingue l'autre.
    [[noreturn]] void refuseReloc(const std::string &what, const Value &v) {
        if (v.byte != Byte::Whole)
            fail("nothing can be computed from high() or low(): they take one byte "
                 "of an address, so the offset goes INSIDE — write high(label + 1), "
                 "not high(label) + 1");
        std::string hint;
        if (what == "'>>'") hint = "; use high() to take its high byte";
        else if (what == "'&'") hint = "; use low() to take its low byte";
        fail(what + " cannot be applied to a relocatable value: its address is not "
             "known until link time" + hint);
    }

    // Ce qu'un opérateur qui ne calcule qu'en absolu a le droit de lire.
    double known(const Value &v, const std::string &what) {
        if (v.relocatable()) refuseReloc(what, v);
        return v.real;
    }

    // `+` et `-` : les deux seules opérations qui gardent une adresse. Ajouter un
    // nombre la déplace ; soustraire deux adresses d'une MÊME section annule les
    // bases et rend un nombre — c'est ainsi que « fin - debut » continue de
    // mesurer une table. Le reste n'a pas de sens et est refusé plutôt que
    // calculé au hasard : le linker n'a pas encore choisi où poser quoi.
    Value combine(const Value &a, const Value &b, int sign) {
        const std::string what = sign > 0 ? "'+'" : "'-'";
        if (a.byte != Byte::Whole) refuseReloc(what, a);
        if (b.byte != Byte::Whole) refuseReloc(what, b);
        const int cb = sign * b.coeff;
        if (a.coeff != 0 && cb != 0 && a.section != b.section)
            fail("two relocatable values from different sections cannot be combined: "
                 "the distance between them is decided by the linker");
        const int c = a.coeff + cb;
        if (c < -1 || c > 1)
            fail("a relocatable expression is ONE section base plus an offset: it "
                 "cannot carry that base twice");
        Value r;
        r.real = a.real + sign * b.real;
        r.coeff = c;
        r.section = c != 0 ? (a.coeff != 0 ? a.section : b.section) : NoSection;
        // Une adresse est un entier : lui donner une partie fractionnaire
        // n'aurait de sens ni pour le linker, ni pour l'octet émis.
        if (c != 0 && r.real != std::floor(r.real))
            fail("a relocatable value cannot carry a fractional offset");
        return r;
    }

    // --- niveaux de précédence (croissante) ---
    Value logOr() {
        Value v = logAnd();
        for (;;) { skip(); if (eat("||")) { double a = known(v, "'||'"); double b = known(logAnd(), "'||'"); v = num((a != 0 || b != 0) ? 1 : 0); } else break; }
        return v;
    }
    Value logAnd() {
        Value v = bitOr();
        for (;;) { skip(); if (eat("&&")) { double a = known(v, "'&&'"); double b = known(bitOr(), "'&&'"); v = num((a != 0 && b != 0) ? 1 : 0); } else break; }
        return v;
    }
    Value bitOr() {
        Value v = bitXor();
        for (;;) { skip(); if (i+1 < s.size() && s[i]=='|' && s[i+1]=='|') break; if (eat("|") || eatWord("or")) { int64_t a = toInt(known(v, "'|'")); v = num((double)(a | toInt(known(bitXor(), "'|'")))); } else break; }
        return v;
    }
    Value bitXor() {
        Value v = bitAnd();
        for (;;) { skip(); if (eat("^") || eatWord("xor")) { int64_t a = toInt(known(v, "'^'")); v = num((double)(a ^ toInt(known(bitAnd(), "'^'")))); } else break; }
        return v;
    }
    Value bitAnd() {
        Value v = equality();
        for (;;) { skip(); if (i+1 < s.size() && s[i]=='&' && s[i+1]=='&') break; if (eat("&") || eatWord("and")) { int64_t a = toInt(known(v, "'&'")); v = num((double)(a & toInt(known(equality(), "'&'")))); } else break; }
        return v;
    }
    Value equality() {
        Value v = relational();
        for (;;) {
            skip();
            if (eat("==")) { double a = known(v, "'=='"); v = num(a == known(relational(), "'=='")); }
            else if (eat("!=")) { double a = known(v, "'!='"); v = num(a != known(relational(), "'!='")); }
            else break;
        }
        return v;
    }
    Value relational() {
        Value v = shift();
        for (;;) {
            skip();
            if (eat("<=")) { double a = known(v, "'<='"); v = num(a <= known(shift(), "'<='")); }
            else if (eat(">=")) { double a = known(v, "'>='"); v = num(a >= known(shift(), "'>='")); }
            else if (i+1 < s.size() && s[i]=='<' && s[i+1]=='<') break;
            else if (i+1 < s.size() && s[i]=='>' && s[i+1]=='>') break;
            else if (eat("<")) { double a = known(v, "'<'"); v = num(a < known(shift(), "'<'")); }
            else if (eat(">")) { double a = known(v, "'>'"); v = num(a > known(shift(), "'>'")); }
            else break;
        }
        return v;
    }
    Value shift() {
        Value v = additive();
        for (;;) { skip();
            if (eat("<<") || eatWord("shl")) { int64_t a = toInt(known(v, "'<<'")); v = num((double)(a << toInt(known(additive(), "'<<'")))); }
            else if (eat(">>") || eatWord("shr")) { int64_t a = toInt(known(v, "'>>'")); v = num((double)(a >> toInt(known(additive(), "'>>'")))); }
            else break; }
        return v;
    }
    Value additive() {
        Value v = term();
        for (;;) { skip(); if (eat("+")) v = combine(v, term(), +1); else if (eat("-")) v = combine(v, term(), -1); else break; }
        return v;
    }
    Value term() {
        Value v = unary();
        for (;;) {
            skip();
            if (eat("*")) { double a = known(v, "'*'"); v = num(a * known(unary(), "'*'")); }
            else if (eat("/")) { double a = known(v, "'/'"); double d = known(unary(), "'/'"); if (d == 0) fail("division by zero"); v = num(a / d); }
            else if (eat("%") || eatWord("mod")) { int64_t a = toInt(known(v, "'%'")); int64_t d = toInt(known(unary(), "'%'")); if (d == 0) fail("modulo by zero"); v = num((double)(a % d)); }
            // Division entière. '//' n'est PAS retenu pour cet usage : il est
            // réservé au commentaire de ligne. Arrondi vers -infini (floor), pas
            // troncature vers zéro : "div" est le raccourci de floor(a/b).
            else if (eatWord("div")) { double a = known(v, "'div'"); double d = known(unary(), "'div'"); if (d == 0) fail("division by zero"); v = num(std::floor(a / d)); }
            else break;
        }
        return v;
    }
    Value unary() {
        skip();
        // L'unaire moins garde l'affinité : c'est lui qui donne son coefficient
        // -1 à `- label`, et c'est `combine` qui décidera si ce -1 s'annule.
        if (eat("-")) { Value v = unary(); if (v.byte != Byte::Whole) refuseReloc("'-'", v); v.real = -v.real; v.coeff = -v.coeff; return v; }
        if (eat("+")) return unary();
        if (eat("~") || eatWord("not")) return num((double)(~toInt(known(unary(), "'~'"))));
        if (eat("!")) return num(known(unary(), "'!'") == 0 ? 1 : 0);
        return power();
    }
    // Puissance. Plus liante que les unaires — `-2**2` vaut -4, comme partout —
    // et associative à DROITE : `2**3**2` vaut 2**9. L'exposant passe par unary()
    // pour que `2**-1` s'écrive.
    //
    // La graphie est `**` et non `^`, déjà pris par le ou exclusif. term() n'y voit
    // que du feu : unary() consomme `2**3` en entier avant que sa boucle ne cherche
    // un `*`.
    Value power() {
        Value v = primary();
        skip();
        if (eat("**")) { double b = known(v, "'**'"); return num(std::pow(b, known(unary(), "'**'"))); }
        return v;
    }
    Value primary() {
        skip();
        if (eat("(")) { Value v = logOr(); if (!eat(")")) fail("expected ')'"); return v; }
        char c = peek();
        if (c == '\'' || c == '"') return num((double)parseLiteral());
        if (c == '$') {
            // '$' suivi d'un chiffre hexa = nombre ; sinon = symbole (adresse courante)
            if (i + 1 < s.size() && std::isxdigit((unsigned char)s[i + 1])) return num(parseNumber());
            ++i;
            Value out;
            if (resolver && resolver("$", out)) return out;
            fail("unknown symbol '$'");
        }
        if (std::isdigit((unsigned char)c) || c == '%' || c == '#')
            return num(parseNumber());
        if (std::isalpha((unsigned char)c) || c == '_' || c == '.' || c == '@')
            return parseIdentOrCall();
        fail("invalid expression");
    }
    // Un littéral en EXPRESSION doit valoir un nombre, et seul un littéral d'un
    // octet en a un. « ld hl,'ab' » n'est pas un cas à tolérer : aucune
    // convention d'endianness n'est écrite dans le source, et l'assembleur de référence y répond par
    // un zéro silencieux — reproduire ça, c'est émettre du faux sans le dire.
    // Le contexte qui accepte une SUITE d'octets, lui, c'est « db » : là le
    // littéral n'est pas un opérande, et la chaîne décalée s'en charge.
    int64_t parseLiteral() {
        kw::Literal lit = kw::readLiteral(s, i);
        if (!lit.error.empty()) fail(lit.error);
        i = lit.end;
        if (lit.bytes.size() == 1) return (unsigned char)lit.bytes[0];
        if (lit.bytes.empty())
            fail("an empty string literal has no value");
        fail("a string literal of " + std::to_string(lit.bytes.size()) +
             " bytes has no value: only a 1-byte literal has one (to emit the "
             "bytes, use 'db')");
    }
    // Nombre : entier (décimal/hexa/binaire) OU flottant décimal ("0.2", "3.14").
    // Le point décimal n'est reconnu qu'en base 10 (pas de sens en hexa/binaire).
    double parseNumber() {
        skip();
        int base = 10;
        if (s[i] == '$' || s[i] == '#') { base = 16; ++i; }
        else if (s[i] == '%') { base = 2; ++i; }
        else if (i + 1 < s.size() && s[i] == '0' && (s[i+1] == 'x' || s[i+1] == 'X')) { base = 16; i += 2; }
        size_t start = i;
        auto isdig = [&](char ch) -> bool {
            if (base == 16) return std::isxdigit((unsigned char)ch);
            if (base == 2) return ch == '0' || ch == '1';
            return std::isdigit((unsigned char)ch);
        };
        int64_t v = 0;
        while (i < s.size() && isdig(s[i])) {
            int d;
            char ch = s[i];
            if (ch >= '0' && ch <= '9') d = ch - '0';
            else d = std::tolower((unsigned char)ch) - 'a' + 10;
            v = v * base + d;
            ++i;
        }
        if (i == start) fail("invalid number");
        if (base == 10 && i < s.size() && s[i] == '.' && i + 1 < s.size() && std::isdigit((unsigned char)s[i + 1])) {
            double frac = 0, scale = 1;
            ++i;
            while (i < s.size() && std::isdigit((unsigned char)s[i])) { frac = frac * 10 + (s[i] - '0'); scale *= 10; ++i; }
            return (double)v + frac / scale;
        }
        return (double)v;
    }
    // Fonctions usuelles reconnues, à une divergence près : les angles de sin/cos sont
    // en RADIANS, pas en degrés comme l'assembleur de référence (ADR 0021).
    //
    // high()/low() sont l'entrée EXPLICITE de l'extraction d'octet sur une
    // adresse qu'on ne connaît pas encore : les seules fonctions qui acceptent
    // une valeur relocalisable, et la raison pour laquelle `label >> 8` peut
    // être refusé sans laisser l'auteur sans réponse. hi()/lo() en sont des
    // graphies — une seule règle à retenir, et le refus nomme quand même la
    // graphie canonique. Sur une valeur ABSOLUE, les quatre calculent tout de
    // suite comme elles l'ont toujours fait.
    bool callBuiltin(const std::string &upperName, Value &out) {
        static const std::set<std::string> unary1 = {
            "SIN", "COS", "ABS", "HI", "LO", "HIGH", "LOW",
            // Arrondis explicites. FLOOR vaut 99 usages dans le corpus — de loin le
            // plus gros manque mesuré — parce que '/' est une division flottante :
            // qui veut tronquer doit l'écrire. ROUND délègue à toInt(), donc il suit
            // automatiquement la règle de départage retenue.
            "FLOOR", "CEIL", "INT", "ROUND",
        };
        static const std::set<std::string> binary2 = { "MIN", "MAX" };
        const bool is1 = unary1.count(upperName) != 0;
        const bool is2 = binary2.count(upperName) != 0;
        if (!is1 && !is2) return false;
        if (!eat("(")) fail("expected '(' after " + upperName);
        Value va = logOr();
        Value vb;
        if (is2) { if (!eat(",")) fail(upperName + ": expected ',' (2 arguments)"); vb = logOr(); }
        if (!eat(")")) fail("expected ')'");

        const std::string self = "'" + lower(upperName) + "()'";
        const bool isHigh = (upperName == "HIGH" || upperName == "HI");
        const bool isLow  = (upperName == "LOW"  || upperName == "LO");
        if ((isHigh || isLow) && va.relocatable()) {
            if (va.byte != Byte::Whole) refuseReloc(self, va);
            if (va.coeff != 1)
                fail(lower(upperName) + "() takes an address, not its negation");
            if (va.real != std::floor(va.real))
                fail("a relocatable value cannot carry a fractional offset");
            out = va;
            out.byte = isHigh ? Byte::High : Byte::Low;
            return true;
        }

        const double a = known(va, self);
        const double b = is2 ? known(vb, self) : 0;
        double r;
        if (upperName == "SIN") r = std::sin(a);        // radians (ADR 0021)
        else if (upperName == "COS") r = std::cos(a);   // radians (ADR 0021)
        else if (upperName == "ABS") r = std::fabs(a);
        else if (isHigh) r = (double)((toInt(a) >> 8) & 0xFF);
        else if (isLow) r = (double)(toInt(a) & 0xFF);
        else if (upperName == "FLOOR") r = std::floor(a);   // vers -infini
        else if (upperName == "CEIL") r = std::ceil(a);     // vers +infini
        else if (upperName == "INT") r = std::trunc(a);     // vers zéro
        else if (upperName == "ROUND") r = (double)toInt(a);
        else if (upperName == "MIN") r = a < b ? a : b;
        else r = a > b ? a : b; // MAX
        out = num(r);
        return true;
    }
    Value parseIdentOrCall() {
        size_t start = i;
        while (i < s.size() && (std::isalnum((unsigned char)s[i]) || s[i]=='_' || s[i]=='.' || s[i]=='@')) ++i;
        std::string name = s.substr(start, i - start);
        std::string up = name; for (char &c : up) c = (char)std::toupper((unsigned char)c);
        size_t save = i;
        if (peek() == '(') {
            Value out;
            if (callBuiltin(up, out)) return out;
            i = save; // pas une fonction connue : laisse '(' pour l'appelant (ne devrait pas arriver ici)
        }
        Value out;
        if (resolver && resolver(name, out)) return out;
        fail("unknown symbol '" + name + "'");
    }
};

} // namespace

Result eval(const std::string &text, const Resolver &resolver) {
    Result r;
    try {
        Parser p(text, resolver);
        Value v = p.logOr();
        p.skip();
        if (p.i < text.size()) { r.ok = false; r.error = "unexpected character: '" + std::string(1, text[p.i]) + "'"; return r; }
        static_cast<Value &>(r) = v;
        // L'invariant que l'appelant peut lire sans y penser : une valeur absolue
        // ne cite AUCUNE section et ne retient aucun octet. `combine` le tient
        // déjà ; le poser ici couvre aussi un résolveur qui remplirait la section
        // d'un symbole absolu.
        if (r.absolute()) { r.section = NoSection; r.byte = Byte::Whole; }
        // `value` est la partie CONNUE, arrondie : la valeur tout court quand
        // elle est absolue, l'addend de la relocalisation quand elle ne l'est pas.
        r.value = toInt(v.real);
        r.ok = true;
    } catch (const EvalError &e) {
        r.ok = false; r.error = e.msg;
    }
    return r;
}

} // namespace expr
