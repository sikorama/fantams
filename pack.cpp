// pack.cpp - Le builder : découpage, en-tête AMSDOS, disquette DSK (ADR 0007)
#include "pack.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace pack {

namespace {

// --- Morceaux ------------------------------------------------------------

struct Piece {
    int bank = 0;
    int addr = 0; // adresse de rangement de l'octet 0, dans sa fenêtre
    std::vector<uint8_t> bytes;
};

// Un bloc des 64 K de base, vu à sa fenêtre naturelle.
bool linear(int bank, int addr) { return bank < 4 && bank == ((addr >> 14) & 3); }

// L'espace d'un emplacement : les 64 K de base n'en font qu'un (-1), chaque
// autre banque a le sien.
int space(int bank, int addr) { return linear(bank, addr) ? -1 : bank; }

// Un morceau par emplacement d'un seul tenant. Le linker pose un bloc par
// fragment, et coupe en plus un fragment des 64 K de base à chaque frontière
// de 16 K (la banque y suit l'adresse) : deux blocs contigus en rangement
// redeviennent un seul morceau s'ils sont dans la même banque, ou tous deux
// dans les 64 K de base. L'ordre de pose n'est pas celui des adresses : les
// blocs sont d'abord triés par emplacement.
std::vector<Piece> split(const link::Image &img) {
    // Les 64 K de base forment un seul espace ; chaque banque d'extension, le sien.
    auto space = [](const link::Block &b) { return pack::space(b.bank, b.addr); };
    std::vector<const link::Block *> sorted;
    for (const link::Block &b : img.blocks) sorted.push_back(&b);
    std::stable_sort(sorted.begin(), sorted.end(), [&](const link::Block *a, const link::Block *b) {
        return std::make_pair(space(*a), a->addr) < std::make_pair(space(*b), b->addr);
    });
    std::vector<Piece> out;
    int lastSpace = 0;
    for (const link::Block *b : sorted) {
        if (!out.empty() && space(*b) == lastSpace &&
            out.back().addr + (int)out.back().bytes.size() == b->addr) {
            out.back().bytes.insert(out.back().bytes.end(), b->bytes.begin(), b->bytes.end());
            continue;
        }
        Piece p;
        p.bank = b->bank;
        p.addr = b->addr;
        p.bytes = b->bytes;
        out.push_back(p);
        lastSpace = space(*b);
    }
    return out;
}

// Le nom d'un morceau seul : celui du source principal, sans chemin ni
// extension, réduit à ce qu'AMSDOS accepte.
std::string programName(const std::string &path) {
    const size_t slash = path.find_last_of("/\\");
    std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
    const size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    std::string out;
    for (char c : base) {
        const char u = (char)std::toupper((unsigned char)c);
        if ((u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '_' || u == '-') out += u;
        if (out.size() == 8) break;
    }
    return out;
}

// Le nom dérivé de l'emplacement (ADR 0007) : « B » + banque en deux chiffres
// hexa + « _ » + adresse en quatre, soit les 8 caractères d'AMSDOS.
std::string derivedName(const Piece &p) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "B%02X_%04X", p.bank & 0xFF, p.addr & 0xFFFF);
    return buf;
}

// --- Encapsulation AMSDOS --------------------------------------------------

std::vector<uint8_t> amsdos(const std::string &name, const std::string &ext, int load,
                            int entry, const std::vector<uint8_t> &bytes) {
    std::vector<uint8_t> f(128, 0);
    f[0] = 0; // user
    for (int i = 0; i < 8; ++i) f[1 + i] = i < (int)name.size() ? (uint8_t)name[i] : ' ';
    for (int i = 0; i < 3; ++i) f[9 + i] = i < (int)ext.size() ? (uint8_t)ext[i] : ' ';
    f[18] = 2; // binaire
    f[21] = (uint8_t)load;
    f[22] = (uint8_t)(load >> 8);
    f[23] = 0xFF;
    const size_t len = bytes.size();
    f[24] = (uint8_t)len;
    f[25] = (uint8_t)(len >> 8);
    f[26] = (uint8_t)entry;
    f[27] = (uint8_t)(entry >> 8);
    f[64] = (uint8_t)len;
    f[65] = (uint8_t)(len >> 8);
    f[66] = (uint8_t)(len >> 16);
    unsigned sum = 0;
    for (int i = 0; i <= 66; ++i) sum += f[i];
    f[67] = (uint8_t)sum;
    f[68] = (uint8_t)(sum >> 8);
    f.insert(f.end(), bytes.begin(), bytes.end());
    return f;
}

// --- Disquette, format DATA ---------------------------------------------
//
// 40 pistes, une face, 9 secteurs de 512 octets numérotés &C1..&C9. Le contenu
// est tenu à plat, dans l'ordre logique (piste, puis ID de secteur) : un bloc
// de 1 K est deux secteurs consécutifs dans cet ordre. Le répertoire occupe les
// blocs 0 et 1 (64 entrées de 32 octets).

constexpr int kTracks = 40;
constexpr int kSectors = 9;
constexpr int kSectorSize = 512;
constexpr int kBlockSize = 1024;
constexpr int kBlocks = kTracks * kSectors * kSectorSize / kBlockSize; // 180
constexpr int kDirBlocks = 2;
constexpr int kDirEntries = kDirBlocks * kBlockSize / 32; // 64

struct Volume {
    std::vector<uint8_t> data = std::vector<uint8_t>((size_t)kBlocks * kBlockSize, 0xE5);

    uint8_t *entry(int i) { return &data[(size_t)i * 32]; }

    // Charge une disquette existante, standard (MV - CPCEMU) ou EXTENDED. Seul
    // le format DATA est compris ; tout autre est refusé plutôt que réécrit.
    bool load(const std::vector<uint8_t> &d, std::string &error) {
        if (d.size() < 256) {
            error = "fichier trop court pour une disquette";
            return false;
        }
        const bool extended = std::memcmp(d.data(), "EXTENDED", 8) == 0;
        if (!extended && std::memcmp(d.data(), "MV - CPC", 8) != 0) {
            error = "ce n'est pas une image de disquette (ni MV - CPCEMU, ni EXTENDED)";
            return false;
        }
        const int tracks = d[48], sides = d[49];
        if (sides != 1) {
            error = "disquette double face : seul le format DATA simple face est lu";
            return false;
        }
        if (tracks != kTracks) {
            error = "disquette de " + std::to_string(tracks) + " pistes : le format DATA en a 40";
            return false;
        }
        size_t pos = 256;
        for (int t = 0; t < tracks; ++t) {
            const size_t tsize = extended ? (size_t)d[52 + t] * 256 : (size_t)(d[50] | d[51] << 8);
            if (tsize < 256 || pos + tsize > d.size() || std::memcmp(&d[pos], "Track-Info", 10) != 0) {
                error = "piste " + std::to_string(t) + " illisible";
                return false;
            }
            const uint8_t *ti = &d[pos];
            if (ti[21] != kSectors) {
                error = "piste " + std::to_string(t) + " : pas 9 secteurs, ce n'est pas le format DATA";
                return false;
            }
            bool seen[kSectors] = {};
            size_t at = pos + 256;
            for (int s = 0; s < kSectors; ++s) {
                const uint8_t *si = ti + 24 + 8 * s;
                const size_t len = extended ? (size_t)(si[6] | si[7] << 8) : (size_t)(128u << ti[20]);
                const int id = si[2] - 0xC1;
                if (id < 0 || id >= kSectors || seen[id] || si[3] != 2 || len != (size_t)kSectorSize ||
                    at + len > pos + tsize) {
                    error = "piste " + std::to_string(t) +
                            " : secteurs hors du format DATA (&C1..&C9, 512 octets)";
                    return false;
                }
                seen[id] = true;
                std::memcpy(&data[((size_t)t * kSectors + id) * kSectorSize], &d[at], kSectorSize);
                at += len;
            }
            pos += tsize;
        }
        return true;
    }

    // Efface le fichier NOM.EXT du user 0, sans tenir compte de la casse ni des
    // attributs (bit 7 du nom et de l'extension). Ses blocs redeviennent libres.
    void erase(const std::string &name, const std::string &ext) {
        char want[11];
        for (int i = 0; i < 8; ++i) want[i] = i < (int)name.size() ? name[i] : ' ';
        for (int i = 0; i < 3; ++i) want[8 + i] = i < (int)ext.size() ? ext[i] : ' ';
        for (int i = 0; i < kDirEntries; ++i) {
            uint8_t *e = entry(i);
            if (e[0] != 0) continue;
            bool same = true;
            for (int k = 0; k < 11 && same; ++k)
                same = std::toupper(e[1 + k] & 0x7F) == std::toupper((unsigned char)want[k]);
            if (same) e[0] = 0xE5;
        }
    }

    bool blockUsed(int b) {
        if (b < kDirBlocks) return true;
        for (int i = 0; i < kDirEntries; ++i) {
            const uint8_t *e = entry(i);
            if (e[0] == 0xE5) continue;
            for (int k = 16; k < 32; ++k)
                if (e[k] == b) return true;
        }
        return false;
    }

    // Écrit un fichier ; faux si la place ou les entrées manquent.
    bool write(const std::string &name, const std::string &ext, const std::vector<uint8_t> &f) {
        const size_t nblocks = (f.size() + kBlockSize - 1) / kBlockSize;
        std::vector<int> blocks;
        for (int b = kDirBlocks; b < kBlocks && blocks.size() < nblocks; ++b)
            if (!blockUsed(b)) blocks.push_back(b);
        if (blocks.size() < nblocks) return false;
        const size_t nextents = nblocks == 0 ? 1 : (nblocks + 15) / 16;
        std::vector<int> slots;
        for (int i = 0; i < kDirEntries && slots.size() < nextents; ++i)
            if (entry(i)[0] == 0xE5) slots.push_back(i);
        if (slots.size() < nextents) return false;

        for (size_t k = 0; k < blocks.size(); ++k) {
            const size_t from = k * kBlockSize;
            const size_t n = std::min((size_t)kBlockSize, f.size() - from);
            uint8_t *dst = &data[(size_t)blocks[k] * kBlockSize];
            std::memset(dst, 0, kBlockSize);
            std::memcpy(dst, &f[from], n);
        }
        const size_t records = (f.size() + 127) / 128;
        for (size_t x = 0; x < nextents; ++x) {
            uint8_t *e = entry(slots[x]);
            std::memset(e, 0, 32);
            for (int i = 0; i < 8; ++i) e[1 + i] = i < (int)name.size() ? (uint8_t)name[i] : ' ';
            for (int i = 0; i < 3; ++i) e[9 + i] = i < (int)ext.size() ? (uint8_t)ext[i] : ' ';
            e[12] = (uint8_t)x;
            const size_t rec = std::min((size_t)128, records - x * 128);
            e[15] = (uint8_t)rec;
            for (size_t k = 0; k < 16 && x * 16 + k < blocks.size(); ++k)
                e[16 + k] = (uint8_t)blocks[x * 16 + k];
        }
        return true;
    }

    // L'image EXTENDED : en-tête de disquette, puis chaque piste. Les secteurs
    // sont posés dans l'entrelacement d'un format DATA d'AMSDOS.
    std::vector<uint8_t> serialize() const {
        static const int order[kSectors] = {0, 5, 1, 6, 2, 7, 3, 8, 4};
        const int trackSize = 256 + kSectors * kSectorSize;
        std::vector<uint8_t> out(256, 0);
        const char *sig = "EXTENDED CPC DSK File\r\nDisk-Info\r\n";
        std::memcpy(out.data(), sig, std::strlen(sig));
        const char *creator = "fantams";
        std::memcpy(out.data() + 34, creator, std::strlen(creator));
        out[48] = kTracks;
        out[49] = 1;
        for (int t = 0; t < kTracks; ++t) out[52 + t] = (uint8_t)(trackSize / 256);
        for (int t = 0; t < kTracks; ++t) {
            std::vector<uint8_t> ti(256, 0);
            const char *tsig = "Track-Info\r\n";
            std::memcpy(ti.data(), tsig, std::strlen(tsig));
            ti[16] = (uint8_t)t;
            ti[17] = 0;
            ti[20] = 2; // 512 octets
            ti[21] = kSectors;
            ti[22] = 0x4E; // GAP#3
            ti[23] = 0xE5; // remplissage
            for (int s = 0; s < kSectors; ++s) {
                uint8_t *si = &ti[24 + 8 * s];
                si[0] = (uint8_t)t;
                si[1] = 0;
                si[2] = (uint8_t)(0xC1 + order[s]);
                si[3] = 2;
                si[6] = (uint8_t)(kSectorSize & 0xFF);
                si[7] = (uint8_t)(kSectorSize >> 8);
            }
            out.insert(out.end(), ti.begin(), ti.end());
            for (int s = 0; s < kSectors; ++s) {
                const size_t off = ((size_t)t * kSectors + order[s]) * kSectorSize;
                out.insert(out.end(), data.begin() + off, data.begin() + off + kSectorSize);
            }
        }
        return out;
    }
};

} // namespace

Result package(const link::Image &img, const Dsk &dsk) {
    Result r;
    const std::vector<Piece> pieces = split(img);
    Volume vol;
    std::string error;
    if (!dsk.existing.empty() && !vol.load(dsk.existing, error)) {
        r.ok = false;
        r.errors.push_back({"", 0, "disquette existante : " + error});
        return r;
    }
    const int runSpace = space(img.runBank, img.runAddress);
    bool entryPlaced = false;
    for (const Piece &p : pieces) {
        std::string name = pieces.size() == 1 ? programName(dsk.program) : "";
        if (name.empty()) name = derivedName(p);
        // Le `run` est une adresse logique, le chargement se fait à l'adresse de
        // rangement : un seul morceau peut la contenir, ou aucun. L'adresse ne
        // suffit pas — `&4000` est aussi bien `main` qu'une banque d'extension
        // vue en `w1` — : le morceau doit aussi être dans l'espace du `run`.
        const bool hasEntry = !entryPlaced && runSpace == space(p.bank, p.addr) &&
                              img.runAddress >= p.addr &&
                              img.runAddress < p.addr + (int)p.bytes.size();
        entryPlaced = entryPlaced || hasEntry;
        const std::vector<uint8_t> f =
            amsdos(name, "BIN", p.addr, hasEntry ? img.runAddress : 0, p.bytes);
        vol.erase(name, "BIN");
        if (!vol.write(name, "BIN", f)) {
            r.ok = false;
            r.errors.push_back({"", 0, "disquette pleine : " + name + ".BIN (" + std::to_string(f.size()) + " octets) n'y tient pas"});
        }
    }
    if (!r.ok) return r;
    if (!pieces.empty() && !entryPlaced) {
        char msg[160];
        std::snprintf(msg, sizeof msg,
                      "le run &%04X ne tombe dans aucun fichier de la disquette : "
                      "les points d'entree AMSDOS sont a 0",
                      img.runAddress);
        r.warnings.push_back({"", 0, msg});
    }
    r.artifacts.push_back({"", vol.serialize()});
    return r;
}

} // namespace pack
