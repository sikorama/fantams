// opcode.cpp - voir opcode.h (ADR 0031)
#include "opcode.h"

#include "expr.h"
#include "parser.h"
#include "z80.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <vector>

namespace opcode {

namespace {

std::string upper(std::string s) {
    for (char &c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

// « d » et « e » ne sont sans ambiguïté qu'À L'INTÉRIEUR de "(ix+d)"/"(iy+d)",
// où le parseur les prend comme texte brut sans passer par la reconnaissance
// de registre. En position NUE (ex. la cible de jr/djnz), le parseur lit
// d'abord un registre : « e » y redevient le registre E, jamais ce
// placeholder — utiliser « n »/« nn »/« imm »/« imm8 »/« imm16 » là où aucune
// des sept graphies ne nomme un registre.
bool isPlaceholder(const std::string &expr) {
    static const std::set<std::string> kNames = {"N", "NN", "D", "E", "IMM", "IMM8", "IMM16"};
    return kNames.count(upper(trim(expr))) != 0;
}

// BIT/RES/SET/RST/IM : le seul cas où un opérande numérique change l'octet
// d'opcode lui-même plutôt que de produire un octet séparé (ADR 0031).
bool isEmbeddedOperandA(z80::Mnemo m) {
    return m == z80::Mnemo::BIT || m == z80::Mnemo::RES || m == z80::Mnemo::SET ||
           m == z80::Mnemo::RST || m == z80::Mnemo::IM;
}

// Un opérande qui porte une expression : Imm, MemImm, ou le déplacement d'Indexed.
bool hasExprSlot(const z80::Operand &o) {
    return o.kind == z80::Operand::Kind::Imm || o.kind == z80::Operand::Kind::MemImm ||
           o.kind == z80::Operand::Kind::Indexed;
}

// Valide qu'un opérande porte la bonne graphie pour sa position : un
// placeholder pour un octet séparé, une valeur littérale pour un octet
// embarqué dans l'opcode. Rend un message d'erreur, vide si c'est bon.
std::string validateOperand(const z80::Operand &o, bool embedded) {
    if (!hasExprSlot(o)) return "";
    const bool ph = isPlaceholder(o.expr);
    if (embedded) {
        if (ph)
            return "'" + o.expr + "' is a placeholder, but this operand is embedded in the "
                   "opcode byte itself: it needs a real numeric value";
    } else if (!ph) {
        static const expr::Resolver kNoSymbols = [](const std::string &, expr::Value &) { return false; };
        expr::Result r = expr::eval(o.expr, kNoSymbols);
        if (r.ok)
            return "'" + o.expr + "' is a literal value where opcode() expects a placeholder "
                   "(n, nn, d, e, imm, imm8 or imm16): its byte is never fixed";
        return "'" + o.expr + "' is neither a placeholder nor a number: opcode() resolves no "
               "symbol";
    }
    return "";
}

struct SandboxCtx : z80::IAsmContext {
    int64_t placeholderValue;
    std::vector<uint8_t> bytes;
    std::string err;

    void emit(uint8_t b) override { bytes.push_back(b); }
    int64_t eval(const std::string &e) override {
        if (isPlaceholder(e)) return placeholderValue;
        static const expr::Resolver kNoSymbols = [](const std::string &, expr::Value &) { return false; };
        expr::Result r = expr::eval(e, kNoSymbols);
        if (!r.ok) { error("'" + e + "': " + r.error); return 0; }
        return r.value;
    }
    uint16_t pc() const override { return 0; }
    void error(const std::string &msg) override { if (err.empty()) err = msg; }
};

// Encode `instrText` avec `placeholderValue` substitué à tout placeholder.
// Renvoie false si le message d'erreur (`err`) doit être remonté tel quel.
bool runEncode(const z80::Instruction &instr, int64_t placeholderValue,
               std::vector<uint8_t> &bytesOut, std::string &err) {
    SandboxCtx ctx;
    ctx.placeholderValue = placeholderValue;
    bool ok = z80::encode(ctx, instr);
    if (!ok || !ctx.err.empty()) { err = ctx.err; return false; }
    bytesOut = ctx.bytes;
    return true;
}

} // namespace

Result extract(const std::string &instrText, int index, int len) {
    Result r;
    parser::Result pr = parser::parseLine(instrText);
    if (!pr.ok || !pr.isInstruction) {
        r.error = "'" + instrText + "' is not a Z80 instruction";
        return r;
    }

    const bool embeddedA = isEmbeddedOperandA(pr.instr.mnemo);
    std::string verr = validateOperand(pr.instr.a, embeddedA);
    if (verr.empty()) verr = validateOperand(pr.instr.b, false);
    if (!verr.empty()) { r.error = verr; return r; }

    // Deux passes avec des valeurs de placeholder distinctes : tout octet qui
    // en dépend diffère d'une passe à l'autre, tout octet fixe reste identique
    // (ADR 0031). -1 est valide dans les trois bornes que l'encodeur applique :
    // [-128,255] (fitsByte), [-128,127] (déplacement indexé), et n'importe
    // quoi tient sur 16 bits.
    std::vector<uint8_t> bytes0, bytes1;
    std::string err;
    if (!runEncode(pr.instr, 0, bytes0, err) || !runEncode(pr.instr, -1, bytes1, err)) {
        r.error = err.empty() ? "cannot encode '" + instrText + "'" : err;
        return r;
    }
    if (bytes0.size() != bytes1.size()) {
        r.error = "'" + instrText + "' does not encode to a fixed size";
        return r;
    }

    const int total = (int)bytes0.size();
    const int n = std::abs(len);
    const bool reversed = len < 0;
    if (index < 0 || n < 0 || index + n > total) {
        r.error = "byte " + std::to_string(index) + " is out of range for '" + instrText +
                  "' (" + std::to_string(total) + " byte(s))";
        return r;
    }
    for (int i = index; i < index + n; ++i) {
        if (bytes0[i] != bytes1[i]) {
            r.error = "byte " + std::to_string(i) + " of '" + instrText + "' is not fixed: "
                      "it depends on a placeholder or an unresolved value";
            return r;
        }
    }

    int64_t v = 0;
    if (!reversed) for (int i = 0; i < n; ++i) v = (v << 8) | bytes0[index + i];
    else for (int i = n - 1; i >= 0; --i) v = (v << 8) | bytes0[index + i];
    r.ok = true;
    r.value = v;
    return r;
}

} // namespace opcode
