// fo.cpp - Le fichier objet (voir fo.h)
//
// Une ligne par fait, un mot-clé en tête, et des paires `clé=valeur` derrière.
// Les octets d'un fragment se donnent par RUNS partageant leur ligne d'origine —
// « data <site> <hexa> » — ce qui porte la provenance sans un nombre par octet,
// et « gap <n> » pour ce qu'un `ds` a réservé sans l'écrire.
#include "fo.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

namespace fo {
namespace {

const char *kMagic = "fantams-object";
const int kVersion = 1;
const uint16_t kUnknownSite = 0xFFFF;

std::string hex(int64_t v) {
    char b[32];
    if (v < 0) snprintf(b, sizeof b, "-0x%llX", (unsigned long long)(-(v + 1)) + 1ULL);
    else snprintf(b, sizeof b, "0x%llX", (unsigned long long)v);
    return b;
}

// Un chemin ou un nom entre guillemets, avec les deux seuls échappements dont
// un chemin ait besoin. Sans guillemets, un nom contenant une espace couperait
// la ligne en deux et le relecteur n'aurait aucun moyen de s'en apercevoir.
std::string quoted(const std::string &s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\';
        o += c;
    }
    return o + "\"";
}

const char *kindName(asmb::Reloc::Kind k) {
    switch (k) {
        case asmb::Reloc::Abs16: return "abs16";
        case asmb::Reloc::Rel8:  return "rel8";
        case asmb::Reloc::High8: return "high8";
        default:                 return "low8";
    }
}

// --- Lecture : un découpeur de ligne en jetons ------------------------------
// Les jetons sont séparés par des espaces, sauf à l'intérieur de guillemets.
// Un `;` hors guillemets ouvre un commentaire jusqu'à la fin de la ligne : le
// fichier porte les siens, et un lecteur doit pouvoir en ajouter.
bool tokenize(const std::string &line, std::vector<std::string> &out, std::string &err) {
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
        if (i >= line.size()) break;
        if (line[i] == ';') break;
        if (line[i] == '"') {
            std::string s;
            ++i;
            for (;;) {
                if (i >= line.size()) { err = "unterminated quoted string"; return false; }
                if (line[i] == '"') { ++i; break; }
                if (line[i] == '\\' && i + 1 < line.size()) ++i;
                s += line[i++];
            }
            out.push_back(s);
            continue;
        }
        // Un jeton nu peut contenir une portion entre guillemets — c'est le cas
        // de `cle="valeur"` — et ce sont ces guillemets-la qui protegent une
        // espace dans un chemin. Le jeton va donc jusqu'a l'espace SUIVANTE
        // hors guillemets.
        size_t j = i;
        while (j < line.size() && !std::isspace((unsigned char)line[j]) && line[j] != ';') {
            if (line[j] == '"') {
                ++j;
                while (j < line.size() && line[j] != '"') {
                    if (line[j] == '\\' && j + 1 < line.size()) ++j;
                    ++j;
                }
                if (j >= line.size()) { err = "unterminated quoted string"; return false; }
            }
            ++j;
        }
        out.push_back(line.substr(i, j - i));
        i = j;
    }
    return true;
}

// Un jeton `clé=valeur`. La valeur peut être un nom entre guillemets, auquel cas
// le découpeur l'a déjà rendue comme un jeton séparé : on recolle ici.
std::string unquote(const std::string &v) {
    if (v.size() < 2 || v.front() != '"' || v.back() != '"') return v;
    std::string o;
    for (size_t i = 1; i + 1 < v.size(); ++i) {
        if (v[i] == '\\' && i + 2 < v.size()) ++i;
        o += v[i];
    }
    return o;
}

bool splitPair(const std::string &tok, std::string &key, std::string &val) {
    const size_t eq = tok.find('=');
    if (eq == std::string::npos || eq == 0) return false;
    key = tok.substr(0, eq);
    val = unquote(tok.substr(eq + 1));
    return true;
}

int64_t toNum(const std::string &s, bool &ok) {
    ok = !s.empty();
    if (!ok) return 0;
    char *end = nullptr;
    const long long v = std::strtoll(s.c_str(), &end, 0);
    ok = end && *end == '\0';
    return (int64_t)v;
}

int64_t fromHexByte(const std::string &s, bool &ok) {
    ok = s.size() == 2 && std::isxdigit((unsigned char)s[0]) && std::isxdigit((unsigned char)s[1]);
    if (!ok) return 0;
    return (int64_t)std::strtol(s.c_str(), nullptr, 16);
}

} // namespace

std::string write(const asmb::Object &obj) {
    std::ostringstream o;
    o << kMagic << ' ' << kVersion << '\n';
    o << "; texte, et volontairement : un objet faux se lit a l'oeil.\n";

    o << "\n; les sections : ce que la source declare, et qui les place\n";
    for (const asmb::Section &s : obj.sections) {
        o << "section " << quoted(s.name) << " id=" << s.id
          << " type=" << (s.kind.empty() ? "-" : s.kind)
          << (s.relocatable ? " reloc" : " abs")
          << " size=" << hex(s.size);
        if (s.hasMax) o << " max=" << hex(s.max);
        o << '\n';
    }

    o << "\n; les lignes qui ont ecrit des octets, citees par les fragments\n";
    for (size_t i = 0; i < obj.sites.size(); ++i)
        o << "site " << i << ' ' << quoted(obj.sites[i].file) << ' ' << obj.sites[i].line << '\n';

    o << "\n; les fragments, dans leur ordre d'ecriture : c'est celui que le\n"
         "; placement rejoue, et sans lui deux recouvrements changeraient d'ordre\n";
    for (size_t fi = 0; fi < obj.fragments.size(); ++fi) {
        const asmb::Fragment &f = obj.fragments[fi];
        o << "fragment " << fi << " section=" << quoted(f.section)
          << (f.placed ? " placed" : " unplaced")
          << " reloc=" << f.relocSection
          << " addr=" << hex(f.addr) << " logical=" << hex(f.logical)
          << " bank=" << f.bank << '\n';
        // Les octets par RUNS partageant leur provenance : une ligne d'objet
        // pour une ligne de source, et la coverage se lit comme « ce qui n'est
        // pas un gap ».
        size_t k = 0;
        while (k < f.bytes.size()) {
            const uint16_t site = f.prov[k];
            size_t j = k;
            while (j < f.bytes.size() && f.prov[j] == site) ++j;
            if (site == 0) {
                o << "  gap " << (j - k) << '\n';
            } else {
                for (size_t p = k; p < j; p += 16) {
                    o << "  data ";
                    if (site == kUnknownSite) o << '-';
                    else o << (site - 1);
                    const size_t end = (p + 16 < j) ? p + 16 : j;
                    for (size_t q = p; q < end; ++q) {
                        char b[8];
                        snprintf(b, sizeof b, " %02X", f.bytes[q]);
                        o << b;
                    }
                    o << '\n';
                }
            }
            k = j;
        }
        o << "end\n";
    }

    o << "\n; ce qu'il manque, et que le linker ecrira\n";
    for (const asmb::Reloc &r : obj.relocs) {
        o << "reloc frag=" << r.frag << " offset=" << hex(r.offset)
          << ' ' << kindName(r.kind);
        if (!r.symbol.empty()) o << " symbol=" << quoted(r.symbol);
        else o << " section=" << r.section;
        o << " addend=" << hex(r.addend) << '\n';
    }

    o << "\n; les symboles. La table de --sym en est un sous-ensemble.\n";
    for (const asmb::Symbol &s : obj.symbolTable) {
        o << "symbol " << quoted(s.name) << (s.isConst ? " const" : " label")
          << (s.isPublic ? " public" : " local")
          << " section=" << quoted(s.section)
          << " value=" << hex(s.value)
          << " frag=" << s.frag << " offset=" << hex(s.offset)
          << " file=" << quoted(s.file) << " line=" << s.line << '\n';
    }

    if (obj.entry.has) {
        o << "\n; le point d'entree : un NOM, que le linker resout\n";
        o << "entry symbol=" << quoted(obj.entry.name)
          << " value=" << hex(obj.entry.value)
          << " file=" << quoted(obj.entry.file) << " line=" << obj.entry.line << '\n';
    }
    return o.str();
}

bool read(const std::string &text, asmb::Object &out, std::string &error) {
    out = asmb::Object();
    std::istringstream in(text);
    std::string line;
    int lineNo = 0;
    bool sawMagic = false;
    int openFrag = -1;   // index du fragment en cours de lecture, -1 hors bloc

    auto fail = [&](const std::string &msg) {
        error = "line " + std::to_string(lineNo) + ": " + msg;
        return false;
    };

    while (std::getline(in, line)) {
        ++lineNo;
        std::vector<std::string> t;
        std::string terr;
        if (!tokenize(line, t, terr)) return fail(terr);
        if (t.empty()) continue;

        // Les paires `cle=valeur`, une fois pour toutes.
        std::map<std::string, std::string> kv;
        std::vector<std::string> flags;
        for (size_t i = 1; i < t.size(); ++i) {
            std::string k, v;
            if (splitPair(t[i], k, v)) kv[k] = v;
            else flags.push_back(t[i]);
        }
        auto has = [&](const char *f) {
            for (const std::string &s : flags) if (s == f) return true;
            return false;
        };
        auto num = [&](const char *k, int64_t dflt, bool &ok) -> int64_t {
            auto it = kv.find(k);
            if (it == kv.end()) { ok = true; return dflt; }
            return toNum(it->second, ok);
        };

        const std::string &w = t[0];

        if (!sawMagic) {
            if (w != kMagic) return fail("not a fantams object file (expected '" + std::string(kMagic) + "')");
            bool ok = false;
            const int64_t v = t.size() > 1 ? toNum(t[1], ok) : 0;
            if (!ok) return fail("missing or unreadable version number");
            if (v != kVersion) return fail("object format version " + std::to_string(v) +
                                           ", this fantams reads version " + std::to_string(kVersion));
            sawMagic = true;
            continue;
        }

        if (openFrag >= 0) {
            asmb::Fragment &f = out.fragments[(size_t)openFrag];
            if (w == "end") { openFrag = -1; continue; }
            if (w == "gap") {
                bool ok = false;
                const int64_t n = t.size() > 1 ? toNum(t[1], ok) : 0;
                if (!ok || n < 0) return fail("gap: expected a byte count");
                f.bytes.insert(f.bytes.end(), (size_t)n, 0);
                f.prov.insert(f.prov.end(), (size_t)n, 0);
                continue;
            }
            if (w == "data") {
                if (t.size() < 2) return fail("data: expected a site then bytes");
                uint16_t site = kUnknownSite;
                if (t[1] != "-") {
                    bool ok = false;
                    const int64_t s = toNum(t[1], ok);
                    if (!ok || s < 0) return fail("data: '" + t[1] + "' is not a site index");
                    site = (uint16_t)(s + 1);
                }
                for (size_t i = 2; i < t.size(); ++i) {
                    bool ok = false;
                    const int64_t b = fromHexByte(t[i], ok);
                    if (!ok) return fail("data: '" + t[i] + "' is not a two-digit hex byte");
                    f.bytes.push_back((uint8_t)b);
                    f.prov.push_back(site);
                }
                continue;
            }
            return fail("inside a fragment, expected 'data', 'gap' or 'end', got '" + w + "'");
        }

        bool ok = true;
        if (w == "section") {
            if (t.size() < 2) return fail("section: expected a name");
            asmb::Section s;
            s.name = t[1];
            s.id = (int)num("id", -1, ok);
            if (!ok) return fail("section: 'id' is not a number");
            s.kind = kv.count("type") && kv["type"] != "-" ? kv["type"] : std::string();
            s.relocatable = has("reloc");
            s.size = num("size", 0, ok);
            if (!ok) return fail("section: 'size' is not a number");
            if (kv.count("max")) { s.hasMax = true; s.max = num("max", 0, ok); }
            if (!ok) return fail("section: 'max' is not a number");
            out.sections.push_back(std::move(s));
        } else if (w == "site") {
            if (t.size() < 4) return fail("site: expected an index, a file and a line");
            bool okn = false;
            const int64_t idx = toNum(t[1], okn);
            if (!okn || idx < 0) return fail("site: '" + t[1] + "' is not an index");
            if ((size_t)idx != out.sites.size())
                return fail("site " + t[1] + " is out of order (expected " +
                            std::to_string(out.sites.size()) + ")");
            asmb::Site st;
            st.file = t[2];
            st.line = (int)toNum(t[3], okn);
            if (!okn) return fail("site: '" + t[3] + "' is not a line number");
            out.sites.push_back(std::move(st));
        } else if (w == "fragment") {
            if (t.size() < 2) return fail("fragment: expected an index");
            bool okn = false;
            const int64_t idx = toNum(t[1], okn);
            if (!okn || idx < 0) return fail("fragment: '" + t[1] + "' is not an index");
            if ((size_t)idx != out.fragments.size())
                return fail("fragment " + t[1] + " is out of order (expected " +
                            std::to_string(out.fragments.size()) + ")");
            asmb::Fragment f;
            f.section = kv.count("section") ? kv["section"] : std::string();
            f.placed = has("placed");
            f.relocSection = (int)num("reloc", -1, ok);
            f.addr = (int)num("addr", 0, ok);
            f.logical = (int)num("logical", 0, ok);
            f.bank = (int)num("bank", -1, ok);
            if (!ok) return fail("fragment: a numeric field is unreadable");
            out.fragments.push_back(std::move(f));
            openFrag = (int)out.fragments.size() - 1;
        } else if (w == "reloc") {
            asmb::Reloc r;
            r.frag = (int)num("frag", -1, ok);
            r.offset = (int)num("offset", 0, ok);
            r.section = (int)num("section", -1, ok);
            r.addend = num("addend", 0, ok);
            if (!ok) return fail("reloc: a numeric field is unreadable");
            if (kv.count("symbol")) r.symbol = kv["symbol"];
            bool kindSeen = false;
            for (const std::string &f : flags) {
                if (f == "abs16") { r.kind = asmb::Reloc::Abs16; kindSeen = true; }
                else if (f == "rel8") { r.kind = asmb::Reloc::Rel8; kindSeen = true; }
                else if (f == "high8") { r.kind = asmb::Reloc::High8; kindSeen = true; }
                else if (f == "low8") { r.kind = asmb::Reloc::Low8; kindSeen = true; }
            }
            if (!kindSeen) return fail("reloc: expected one of abs16, rel8, high8, low8");
            out.relocs.push_back(std::move(r));
        } else if (w == "symbol") {
            if (t.size() < 2) return fail("symbol: expected a name");
            asmb::Symbol s;
            s.name = t[1];
            s.isConst = has("const");
            if (!s.isConst && !has("label")) return fail("symbol: expected 'label' or 'const'");
            s.isPublic = has("public");
            s.section = kv.count("section") ? kv["section"] : std::string();
            s.value = num("value", 0, ok);
            s.frag = (int)num("frag", -1, ok);
            s.offset = (int)num("offset", 0, ok);
            if (!ok) return fail("symbol: a numeric field is unreadable");
            if (kv.count("file")) s.file = kv["file"];
            s.line = (int)num("line", 0, ok);
            if (!ok) return fail("symbol: 'line' is not a number");
            out.symbolTable.push_back(std::move(s));
        } else if (w == "entry") {
            out.entry.has = true;
            out.entry.name = kv.count("symbol") ? kv["symbol"] : std::string();
            out.entry.value = num("value", 0, ok);
            if (!ok) return fail("entry: 'value' is not a number");
            if (kv.count("file")) out.entry.file = kv["file"];
            out.entry.line = (int)num("line", 0, ok);
            if (!ok) return fail("entry: 'line' is not a number");
        } else {
            return fail("unknown block '" + w + "'");
        }
    }
    if (!sawMagic) return fail("empty file: expected a '" + std::string(kMagic) + "' header");
    if (openFrag >= 0) return fail("fragment " + std::to_string(openFrag) + " is not closed by 'end'");

    // `symbols` est reconstruite depuis la table exportable : c'est elle qui
    // permet au linker de resoudre un `run <nom>`. Les VARIABLES n'y sont pas,
    // et n'y etaient deja pas — leur valeur n'est celle d'aucun point precis.
    for (const asmb::Symbol &s : out.symbolTable) out.symbols[s.name] = s.value;
    out.ok = true;
    return true;
}

} // namespace fo
