// cpr.cpp - Export cartouche CPC Plus (.cpr)
#include "cpr.h"

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

} // namespace

std::vector<uint8_t> build(const std::map<int, link::Image> &perBank,
                            const profile::Profile &profile, std::string &error) {
    bool hasAxis = false;
    for (const profile::Axis &a : profile.axes)
        if (a.name == "cart_rom") hasAxis = true;
    if (!hasAxis) {
        error = "le profil ne declare pas l'axe 'cart_rom' : rien a mettre en cartouche";
        return {};
    }
    error.clear();

    const profile::Bank *bank = findCartRomBank(profile);
    const int64_t chunkSize = bank && bank->hasSize ? bank->size : 0x4000;
    const int store = bank && bank->hasStore ? (int)bank->store : -1;

    std::vector<std::pair<int, std::vector<uint8_t>>> chunks;  // (n, 16 Ko)
    for (const auto &kv : perBank) {
        const int physicalId = kv.first;
        const link::Image &img = kv.second;
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
        if (wrote) chunks.emplace_back(physicalId, std::move(data));
    }

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

} // namespace cpr
