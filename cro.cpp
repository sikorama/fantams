// cro.cpp - Export conteneur de ROMs (.cro, Logon System)
#include "cro.h"

#include "riff.h"

#include <cctype>
#include <cstdio>

namespace cro {
namespace {

// Une banque du profil, par son nom : le paramètre d'une banque paramétrique
// (`rom_hi<n>`) est retiré par l'analyseur, elle s'y range sous `rom_hi`.
const profile::Bank *findBank(const profile::Profile &profile, const char *name) {
    for (const profile::Bank &b : profile.banks)
        if (b.name == name) return &b;
    return nullptr;
}

// Les octets qu'une image a RÉELLEMENT écrits sur `bank`, à leur place dans
// la banque, zéro ailleurs. `false` si elle n'y a rien écrit.
bool bankBytes(const link::Image &img, const profile::Bank &bank, std::vector<uint8_t> &out) {
    const int64_t size = bank.hasSize ? bank.size : 0x4000;
    out.assign((size_t)size, 0);
    bool wrote = false;
    for (const link::Block &blk : img.blocks) {
        if (!bank.hasStore || blk.bank != (int)bank.store) continue;
        for (size_t k = 0; k < blk.bytes.size(); ++k) {
            if (k >= blk.covered.size() || !blk.covered[k]) continue;
            out[(size_t)((blk.addr + (int)k) & (size - 1))] = blk.bytes[k];
            wrote = true;
        }
    }
    return wrote;
}

// Les banques qu'un CRO sait livrer, et comment chacune devient une ROM
// (ADR 0033). `maxNumber` < 0 : la banque n'a pas de numéro, `--cro-rom` est
// refusé.
struct RomBank {
    const char *bank;
    Family family;
    int maxNumber;
};
const RomBank kRomBanks[] = {
    {"rom_lo", Family::Classic, -1},
    {"rom_hi", Family::Classic, 255},
    {"crom", Family::Cartridge, 31},
};

Rom describe(const RomBank &rb, int n) {
    Rom rom;
    rom.family = rb.family;
    char id[16];
    if (rb.maxNumber < 0) {
        rom.id = "lo";
        rom.type = kLow;
        rom.slot = 0;
        rom.physical = 0;
    } else if (rb.family == Family::Classic) {
        // Sur l'ancienne gamme, le numéro physique est le slot.
        snprintf(id, sizeof id, "hi%02d", n);
        rom.id = id;
        rom.type = kHigh;
        rom.slot = (uint32_t)n;
        rom.physical = (uint32_t)n;
    } else {
        snprintf(id, sizeof id, "cb%02d", n);
        rom.id = id;
        // Seules les ROMs physiques 0..7 se mappent en bas : c'est la limite
        // entre `cart_rom` et `cart_rom_hi`, que l'image ne dit plus.
        rom.type = n < 8 ? kBankable : kHigh;
        // La table de transposition du Plus : le slot 7 est la ROM physique 3.
        rom.slot = n == 3 ? 7 : 1;
        rom.physical = (uint32_t)n;
    }
    return rom;
}

bool isContainer(const std::string &id) { return id == "GRRO" || id == "ROM "; }

riff::Chunk *child(riff::Chunk &parent, const char *id) {
    for (riff::Chunk &c : parent.children)
        if (c.id == id) return &c;
    return nullptr;
}

bool readU32(const riff::Chunk *c, uint32_t &v) {
    if (!c || c->data.size() < 4) return false;
    v = (uint32_t)c->data[0] | ((uint32_t)c->data[1] << 8) | ((uint32_t)c->data[2] << 16) |
        ((uint32_t)c->data[3] << 24);
    return true;
}

// Remplace les données du premier sous-chunk `src.id` de `parent`, ou l'ajoute
// à la fin : ce que `parent` porte d'autre reste à sa place.
void setChild(riff::Chunk &parent, const riff::Chunk &src) {
    if (riff::Chunk *c = child(parent, src.id.c_str())) c->data = src.data;
    else parent.children.push_back(src);
}

riff::Chunk leaf(const char *id, std::vector<uint8_t> data) {
    riff::Chunk c;
    c.id = id;
    c.data = std::move(data);
    return c;
}

riff::Chunk u32Chunk(const char *id, uint32_t v) {
    return leaf(id, {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)});
}

riff::Chunk textChunk(const char *id, const std::string &s) {
    return leaf(id, std::vector<uint8_t>(s.begin(), s.end()));
}

riff::Chunk romChunk(const Rom &rom) {
    riff::Chunk c;
    c.id = "ROM ";
    c.container = true;
    c.children = {textChunk("RID ", rom.id), u32Chunk("RTYP", rom.type),
                  u32Chunk("RLOG", rom.slot), u32Chunk("RPHY", rom.physical),
                  leaf("RDT ", rom.data)};
    return c;
}

// La famille d'une ROM déjà rangée, lue à son RID (ADR 0033) : `false` pour
// un RID que fantams n'a pas pu écrire.
bool familyOf(const std::string &rid, Family &fam) {
    // `hiNN` va jusqu'a `hi255` : deux ou trois chiffres, comme `%02d` les écrit.
    auto numbered = [&](const char *prefix) {
        if (rid.size() < 4 || rid.size() > 5 || rid.compare(0, 2, prefix) != 0) return false;
        for (size_t i = 2; i < rid.size(); ++i)
            if (!std::isdigit((unsigned char)rid[i])) return false;
        return true;
    };
    if (rid == "lo" || numbered("hi")) { fam = Family::Classic; return true; }
    if (numbered("cb")) { fam = Family::Cartridge; return true; }
    return false;
}

const char *familyName(Family fam) { return fam == Family::Classic ? "classique" : "de cartouche"; }

// fantams écrit toujours GNUM, GLBL et GMSK, dans cet ordre : un groupe relu
// sans eux, qu'on touche, reçoit ceux qui manquent, aux défauts du guide CRO.
// Ceux qu'il a déjà restent où ils sont.
void completeHeader(riff::Chunk &group, uint32_t number, const std::string &label) {
    auto indexOf = [&](const char *id) -> long {
        for (size_t i = 0; i < group.children.size(); ++i)
            if (group.children[i].id == id) return (long)i;
        return -1;
    };
    auto ensure = [&](const riff::Chunk &c, long after) {
        if (indexOf(c.id.c_str()) < 0)
            group.children.insert(group.children.begin() + (after + 1), c);
    };
    ensure(u32Chunk("GNUM", number), -1);
    ensure(textChunk("GLBL", label), indexOf("GNUM"));
    ensure(u32Chunk("GMSK", 0xFFFFFFFF), indexOf("GLBL"));
}

} // namespace

bool extractOne(const link::Image &img, const profile::Profile &profile, int romNumber,
                Rom &out, std::string &error) {
    error.clear();
    const RomBank *found = nullptr;
    std::vector<uint8_t> data;
    std::string written;
    int count = 0;
    for (const RomBank &rb : kRomBanks) {
        const profile::Bank *bank = findBank(profile, rb.bank);
        std::vector<uint8_t> bytes;
        if (!bank || !bankBytes(img, *bank, bytes)) continue;
        written += std::string(written.empty() ? "" : ", ") + rb.bank;
        if (count++ == 0) { found = &rb; data = std::move(bytes); }
    }
    if (!found) {
        error = "l'image ne remplit aucune banque de ROM (rom_lo, rom_hi<n>, crom<n>)";
        return false;
    }
    if (count > 1) {
        error = "l'image remplit plusieurs banques de ROM (" + written +
                ") : un .cro se construit une ROM par invocation";
        return false;
    }
    if (found->maxNumber < 0) {
        if (romNumber >= 0) {
            error = std::string("la banque ") + found->bank +
                    " n'a pas de numero : --cro-rom ne s'y applique pas";
            return false;
        }
    } else if (romNumber < 0) {
        error = std::string("la banque ") + found->bank +
                "<n> demande --cro-rom <n>";
        return false;
    } else if (romNumber > found->maxNumber) {
        error = std::string("--cro-rom ") + std::to_string(romNumber) + " hors de 0.." +
                std::to_string(found->maxNumber) + " pour " + found->bank + "<n>";
        return false;
    }
    out = describe(*found, romNumber);
    out.data = std::move(data);
    return true;
}

std::vector<uint8_t> merge(const std::vector<uint8_t> &container, const Rom &rom,
                           const GroupOptions &options, const std::string &defaultLabel,
                           std::string &error) {
    error.clear();
    std::vector<riff::Chunk> top;
    if (!container.empty() && !riff::read(container, "CRO ", isContainer, top, error)) {
        error = "le conteneur existant n'est pas un CRO bien forme : " + error;
        return {};
    }

    // Le groupe `options.group`. Sans GNUM, un groupe prend son rang parmi
    // les GRRO du fichier (guide CRO).
    riff::Chunk *group = nullptr;
    uint32_t order = 0;
    for (riff::Chunk &c : top) {
        if (c.id != "GRRO") continue;
        uint32_t num = order++;
        readU32(child(c, "GNUM"), num);
        if (num == options.group) { group = &c; break; }
    }
    if (group) {
        completeHeader(*group, options.group, defaultLabel);
        // Un groupe existant ne change que sur ce que la CLI a DÉCLARÉ.
        if (options.hasLabel) setChild(*group, textChunk("GLBL", options.label));
        if (options.hasMask) setChild(*group, u32Chunk("GMSK", options.mask));
    } else {
        riff::Chunk g;
        g.id = "GRRO";
        g.container = true;
        g.children = {u32Chunk("GNUM", options.group),
                      textChunk("GLBL", options.hasLabel ? options.label : defaultLabel),
                      u32Chunk("GMSK", options.mask)};
        top.push_back(std::move(g));
        group = &top.back();
    }

    // Un groupe est d'une seule famille : celle qu'on lit au RID des ROMs déjà
    // rangées. Une ROM au RID inconnu, venue d'un autre outil, est neutre.
    for (riff::Chunk &c : group->children) {
        const riff::Chunk *rid = c.id == "ROM " ? child(c, "RID ") : nullptr;
        Family fam;
        if (!rid || !familyOf(std::string(rid->data.begin(), rid->data.end()), fam)) continue;
        if (fam != rom.family) {
            error = "le groupe " + std::to_string(options.group) + " porte des ROMs " +
                    familyName(fam) + " ; " + rom.id + " est une ROM " + familyName(rom.family) +
                    " : un groupe ne mele pas les deux familles (--cro-group)";
            return {};
        }
    }

    // Dans un groupe, une ROM est désignée par son numéro physique — et par le
    // fait d'être la ROM basse : `lo` et `hi00` ont tous deux le numéro 0, et
    // ne se remplacent pas (ADR 0033).
    const riff::Chunk fresh = romChunk(rom);
    for (riff::Chunk &c : group->children) {
        uint32_t phys, type = kHigh;
        if (c.id != "ROM " || !readU32(child(c, "RPHY"), phys) || phys != rom.physical) continue;
        readU32(child(c, "RTYP"), type);
        if ((type == kLow) != (rom.type == kLow)) continue;
        for (const riff::Chunk &field : fresh.children) setChild(c, field);
        return riff::write("CRO ", top);
    }
    group->children.push_back(fresh);
    return riff::write("CRO ", top);
}

} // namespace cro
