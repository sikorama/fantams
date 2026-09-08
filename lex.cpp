// lex.cpp - Le découpeur de jetons et le curseur (voir lex.h)
#include "lex.h"

#include <cctype>
#include <cstdlib>

namespace lex {
namespace {

bool nameStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
bool nameChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

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

} // namespace

bool tokenize(const std::string &text, std::vector<Tok> &out,
              std::string &err, int &errLine) {
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
            const size_t a = i;
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
        // `<<` et `>>` sont des jetons : sans eux, `PAGE << 3` demanderait au
        // parseur d'expressions de recoller deux chevrons, qui servent par
        // ailleurs a porter le parametre d'un etat — `ext_w1<b>`.
        if ((c == '<' || c == '>') && i + 1 < text.size() && text[i + 1] == c) {
            t.kind = Tok::Punct; t.s = std::string(2, c); i += 2;
            out.push_back(std::move(t));
            continue;
        }
        if (std::string("{}[]<>,=.+-*/|&^~()?:").find(c) != std::string::npos) {
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

void Cursor::err(int line, const std::string &msg) {
    ok = false;
    errors.push_back({file, line, msg});
}

std::string Cursor::got() const {
    switch (cur().kind) {
        case Tok::End:    return "end of file";
        case Tok::Number: return "'" + cur().s + "'";
        case Tok::Text:   return "a string";
        default:          return "'" + cur().s + "'";
    }
}

bool Cursor::want(const char *p) {
    if (isPunct(p)) { next(); return true; }
    err(std::string("expected '") + p + "', got " + got());
    return false;
}

bool Cursor::wantNumber(int64_t &v) {
    if (cur().kind == Tok::Number) { v = cur().n; next(); return true; }
    err("expected a number, got " + got());
    return false;
}

bool Cursor::wantName(std::string &v) {
    if (cur().kind == Tok::Name) { v = cur().s; next(); return true; }
    err("expected a name, got " + got());
    return false;
}

void Cursor::skipBlock() {
    int depth = 0;
    for (; !atEnd(); next()) {
        if (isPunct("{")) ++depth;
        else if (isPunct("}") && --depth <= 0) { next(); return; }
    }
}

} // namespace lex
