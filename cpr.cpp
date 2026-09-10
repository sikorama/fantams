// cpr.cpp - Export cartouche CPC Plus (.cpr)
#include "cpr.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace cpr {
namespace {

void putU32LE(std::vector<uint8_t> &out, uint32_t v) {
    out.push_back((uint8_t)(v & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
    out.push_back((uint8_t)((v >> 16) & 0xFF));
    out.push_back((uint8_t)((v >> 24) & 0xFF));
}

void putFourCC(std::vector<uint8_t> &out, const char *cc) {
    for (int i = 0; i < 4; ++i) out.push_back((uint8_t)cc[i]);
}

// La banque `crom<n>` du profil : un STORE fixe pour toute la famille
// paramétrique (cf. cpr.h). Le paramètre est retiré par l'analyseur de
// profil (`profile::parse`) : `crom<n>` s'y range sous le nom `crom`.
const profile::Bank *findCartRomBank(const profile::Profile &profile) {
    for (const profile::Bank &b : profile.banks)
        if (b.name == "crom") return &b;
    return nullptr;
}

// `false` (sans erreur) si l'axe n'existe pas : c'est `error` qui distingue
// « pas d'axe » (les deux appelants le nomment differemment) de « rien
// d'ecrit ». Les deux chemins publics le relisent APRES cet appel.
bool hasCartRomAxis(const profile::Profile &profile) {
    for (const profile::Axis &a : profile.axes)
        if (a.name == "cart_rom") return true;
    return false;
}

// Les 16 Ko d'UNE image sur la banque `crom<n>` du profil — sans dire quel
// `n` : c'est le coeur commun a `build()` (plusieurs images, un `n` par
// entree de `perBank`) et `extractOne()` (une image, `n` inconnu d'ici).
std::pair<bool, std::vector<uint8_t>> bankBytes(const link::Image &img,
                                                 const profile::Profile &profile) {
    const profile::Bank *bank = findCartRomBank(profile);
    const int64_t chunkSize = bank && bank->hasSize ? bank->size : 0x4000;
    const int store = bank && bank->hasStore ? (int)bank->store : -1;

    std::vector<uint8_t> data((size_t)chunkSize, 0);
    bool wrote = false;
    for (const link::Block &blk : img.blocks) {
        if (blk.bank != store) continue;
        for (size_t k = 0; k < blk.bytes.size(); ++k) {
            if (k >= blk.covered.size() || !blk.covered[k]) continue;
            const size_t off = (size_t)((blk.addr + (int)k) & (chunkSize - 1));
            data[off] = blk.bytes[k];
            wrote = true;
        }
    }
    return {wrote, std::move(data)};
}

// Le RIFF/AMS! complet a partir des chunks DEJA EXTRAITS (16 Ko chacun),
// dans l'ordre ou ils apparaissent dans `chunks`.
std::vector<uint8_t> serialize(const std::vector<std::pair<int, std::vector<uint8_t>>> &chunks) {
    std::vector<uint8_t> body;
    putFourCC(body, "AMS!");
    for (const auto &c : chunks) {
        char name[5];
        snprintf(name, sizeof name, "cb%02x", c.first);
        putFourCC(body, name);
        putU32LE(body, (uint32_t)c.second.size());
        body.insert(body.end(), c.second.begin(), c.second.end());
        if (c.second.size() % 2 != 0) body.push_back(0);  // padding pair RIFF
    }

    std::vector<uint8_t> out;
    putFourCC(out, "RIFF");
    putU32LE(out, (uint32_t)body.size());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

// Relit un `.cpr` DEJA ECRIT en chunks `(n, octets)`, dans l'ordre du
// fichier — c'est cet ordre que `merge()` doit preserver pour qu'ajouter une
// banque ne remue pas les autres dans le fichier resultant.
bool parseContainer(const std::vector<uint8_t> &container,
                     std::vector<std::pair<int, std::vector<uint8_t>>> &chunks,
                     std::string &error) {
    chunks.clear();
    if (container.empty()) return true;
    auto fourCC = [&](size_t off, const char *want) {
        return container.size() >= off + 4 &&
               std::equal(want, want + 4, container.begin() + (long)off);
    };
    auto u32 = [&](size_t off) -> uint32_t {
        return (uint32_t)container[off] | ((uint32_t)container[off + 1] << 8) |
               ((uint32_t)container[off + 2] << 16) | ((uint32_t)container[off + 3] << 24);
    };
    if (container.size() < 12 || !fourCC(0, "RIFF") || !fourCC(8, "AMS!")) {
        error = "ce n'est pas un conteneur CPR (RIFF/AMS! attendu)";
        return false;
    }
    size_t pos = 12;
    while (pos + 8 <= container.size()) {
        if (container[pos] != 'c' || container[pos + 1] != 'b' ||
            !std::isxdigit(container[pos + 2]) || !std::isxdigit(container[pos + 3])) {
            error = "chunk mal forme dans le conteneur existant";
            return false;
        }
        const int id = std::stoi(std::string(container.begin() + (long)pos + 2,
                                              container.begin() + (long)pos + 4), nullptr, 16);
        const uint32_t size = u32(pos + 4);
        const size_t dataStart = pos + 8;
        if (dataStart + size > container.size()) {
            error = "chunk tronque dans le conteneur existant";
            return false;
        }
        chunks.emplace_back(id, std::vector<uint8_t>(container.begin() + (long)dataStart,
                                                       container.begin() + (long)(dataStart + size)));
        pos = dataStart + size + (size % 2 != 0 ? 1 : 0);
    }
    return true;
}

} // namespace

std::vector<uint8_t> build(const std::map<int, link::Image> &perBank,
                            const profile::Profile &profile, std::string &error) {
    if (!hasCartRomAxis(profile)) {
        error = "le profil ne declare pas l'axe 'cart_rom' : rien a mettre en cartouche";
        return {};
    }
    error.clear();

    std::vector<std::pair<int, std::vector<uint8_t>>> chunks;  // (n, 16 Ko)
    for (const auto &kv : perBank) {
        auto [wrote, data] = bankBytes(kv.second, profile);
        if (wrote) chunks.emplace_back(kv.first, std::move(data));
    }
    return serialize(chunks);
}

bool extractOne(const link::Image &img, const profile::Profile &profile,
                 std::vector<uint8_t> &bankOut, std::string &error) {
    if (!hasCartRomAxis(profile)) {
        error = "le profil ne declare pas l'axe 'cart_rom' : rien a mettre en cartouche";
        return false;
    }
    error.clear();
    auto [wrote, data] = bankBytes(img, profile);
    if (!wrote) return false;
    bankOut = std::move(data);
    return true;
}

std::vector<uint8_t> merge(const std::vector<uint8_t> &container, int physicalId,
                            const std::vector<uint8_t> &bankBytesIn,
                            const profile::Profile &profile, std::string &error) {
    (void)profile;
    std::vector<std::pair<int, std::vector<uint8_t>>> chunks;
    if (!parseContainer(container, chunks, error)) return {};
    error.clear();

    bool replaced = false;
    for (auto &c : chunks)
        if (c.first == physicalId) { c.second = bankBytesIn; replaced = true; break; }
    if (!replaced) chunks.emplace_back(physicalId, bankBytesIn);

    return serialize(chunks);
}

} // namespace cpr
