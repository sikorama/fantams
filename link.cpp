// link.cpp - Le linker (voir link.h)
//
// Le placement de l'étage B tient en une phrase : chaque fragment va où son
// `org` le dit, dans l'ordre où l'assembleur l'a produit. Rejouer cet ordre
// rejoue les écritures dans leur ordre d'origine — c'est ce qui fait qu'un
// recouvrement se diagnostique là où il s'est produit, et pas ailleurs.
#include "link.h"

#include <cstdio>
#include <map>

namespace link {
namespace {

// Un ESPACE de 16 K, indexé par banque, et non un tableau plat de 64 K : le
// masquage 16 bits ne laisse aucun endroit où loger une banque (ADR 0006).
// Alloué à la première écriture, si bien qu'une source qui n'écrit qu'en banque
// 4 ne paie pas les 64 K de base.
struct Space {
    std::vector<uint8_t> bytes;
    std::vector<uint16_t> prov;   // 0 = jamais ecrit, sinon 1+index dans les sites
    Space() : bytes(0x4000, 0), prov(0x4000, 0) {}
};

static const uint16_t kUnknownSite = 0xFFFF;

// Un chevauchement en cours d'accumulation : les octets consécutifs qui
// partagent le même couple (site écrasé, site écrasant) ne donnent qu'un seul
// avertissement. C'est cette coalescence, et non un plafond, qui empêche un bloc
// réécrit de produire un avertissement par octet.
struct Overlap { bool active = false; int bank = 0, start = 0, end = 0; uint16_t prev = 0, cur = 0; };

// L'état d'un linkage. Une structure et non des paramètres qui circulent : la
// détection de recouvrement porte de la mémoire d'un octet à l'autre.
struct Linker {
    Image out;
    std::map<int, Space> spaces_;
    Overlap ov_;
    int lo_ = 0x10000, hi_ = 0;
    // Les sites de TOUS les objets, mis bout à bout : `prov` d'un fragment est
    // relatif à son objet, et le linker en voit plusieurs.
    std::vector<asmb::Site> sites_;

    std::string siteLabel(uint16_t id) const {
        if (id == 0 || id == kUnknownSite || id > sites_.size()) return "site inconnu";
        const asmb::Site &s = sites_[id - 1];
        return s.file + ":" + std::to_string(s.line);
    }

    void flushOverlap() {
        if (!ov_.active) return;
        ov_.active = false;
        char range[64];
        if (ov_.end - ov_.start == 1) snprintf(range, sizeof range, "&%04X", ov_.start);
        else snprintf(range, sizeof range, "&%04X-&%04X", ov_.start, ov_.end - 1);
        const bool known = ov_.cur != 0 && ov_.cur != kUnknownSite && ov_.cur <= sites_.size();
        std::string where = known ? sites_[ov_.cur - 1].file : std::string();
        int line = known ? sites_[ov_.cur - 1].line : 0;
        // La banque n'est nommee que si elle sort des 64 K de base : la mentionner
        // partout ferait du bruit sur l'immense majorite des sources, qui n'en ont
        // qu'une notion implicite.
        char bk[24] = "";
        if (ov_.bank >= 4) snprintf(bk, sizeof bk, "banque %d, ", ov_.bank);
        out.warnings.push_back({where, line,
            std::string("chevauchement : ") + bk + range + " deja ecrit par " + siteLabel(ov_.prev)});
    }

    // `off` localise l'octet DANS sa banque (c'est la que le recouvrement se
    // produit) ; `addr` est l'adresse de RANGEMENT, celle ou les octets s'ecrasent
    // reellement et donc celle que le diagnostic doit nommer.
    void noteWrite(int bank, int off, int addr, uint16_t site, Space &sp) {
        uint16_t prev = sp.prov[off];
        sp.prov[off] = site;
        if (prev == 0 || prev == site) return;   // écrire sur du vierge, ou se relire soi-même
        if (ov_.active && ov_.bank == bank && ov_.end == addr &&
            ov_.prev == prev && ov_.cur == site) { ov_.end = addr + 1; return; }
        flushOverlap();
        ov_ = {true, bank, addr, addr + 1, prev, site};
    }

    // La banque où ranger l'octet d'adresse `addr` du fragment `f`.
    //
    // Sans préfixe, elle SUIT l'adresse — c'est le comportement historique, et il
    // reste juste : les 64 K de base sont les banques 0..3. Un « org b<n>: » la
    // fixe pour tout le fragment (ADR 0005).
    static int bankOf(const asmb::Fragment &f, int addr) {
        return f.bank < 0 ? ((addr >> 14) & 3) : f.bank;
    }

    // Un BLOC vit dans UNE banque et à des adresses qui se suivent. Un fragment
    // sans préfixe de banque qui franchit une frontière de 16 K en donne donc
    // deux : la banque y suit l'adresse, et c'est au placement que cela se voit.
    void place(const asmb::Object &obj, uint16_t siteBase) {
        for (const asmb::Fragment &f : obj.fragments) {
            Block blk;
            bool open = false;
            for (size_t k = 0; k < f.bytes.size(); ++k) {
                const int addr = (f.addr + (int)k) & 0xFFFF;
                const int bank = bankOf(f, addr);
                const bool follows = open && bank == blk.bank &&
                                     ((blk.addr + (int)blk.bytes.size()) & 0xFFFF) == addr;
                if (!follows) {
                    if (open) out.blocks.push_back(std::move(blk));
                    blk = Block();
                    blk.bank = bank;
                    blk.addr = addr;
                    open = true;
                }
                blk.bytes.push_back(f.bytes[k]);
                blk.covered.push_back(f.prov[k] ? 1 : 0);
                if (f.prov[k] == 0) continue;   // un trou reserve ne se range pas
                const int off = addr & 0x3FFF;   // ADR 0005 : l'offset est le masquage
                const uint16_t site = f.prov[k] == kUnknownSite
                                          ? kUnknownSite : (uint16_t)(f.prov[k] + siteBase);
                Space &sp = spaces_[bank];
                noteWrite(bank, off, addr, site, sp);
                sp.bytes[off] = f.bytes[k];
                // lo_/hi_ ne decrivent que le binaire plat des 64 K de base.
                if (bank < 4) {
                    const int flat = bank * 0x4000 + off;
                    if (flat < lo_) lo_ = flat;
                    if (flat + 1 > hi_) hi_ = flat + 1;
                }
            }
            if (open) out.blocks.push_back(std::move(blk));
        }
    }

    // Un RUN qui tombe dans un bloc deplace fait demarrer le PC sur de la memoire
    // vide : les octets sont ranges ailleurs, en attente d'etre recopies. Le PC
    // reste celui que la source a demande — « run label » doit valoir ce que vaut
    // « label », sinon plus rien n'est previsible — mais le silence laisserait une
    // panne a l'execution sans diagnostic, la meme raison qui fait avertir sur une
    // banque remanente (ADR 0005).
    //
    // C'est ICI que le diagnostic vit maintenant : « déplacé » veut dire « rangé
    // ailleurs que son adresse logique », et cela ne se sait qu'au placement.
    void warnRunDisplaced(const asmb::Object &obj, int run) {
        const int r = run & 0xFFFF;
        for (const asmb::Fragment &f : obj.fragments) {
            if (f.logical == f.addr) continue;   // rien de deplace ici
            for (size_t k = 0; k < f.bytes.size(); ++k) {
                if (f.prov[k] == 0) continue;
                if (((f.logical + (int)k) & 0xFFFF) != r) continue;
                char msg[192];
                snprintf(msg, sizeof msg,
                         "RUN &%04X falls inside a displaced ORG block: the bytes are stored elsewhere, "
                         "so nothing is at this address until a loader copies them there", r);
                out.warnings.push_back({obj.entry.file, obj.entry.line, msg});
                return;
            }
        }
    }
};

} // namespace

Image build(const std::vector<asmb::Object> &objects) {
    Linker lk;
    // Les objets se posent dans l'ordre où on les a donnés. Leurs tables de
    // sites se concatènent, et `prov` se décale d'autant : c'est ce qui laisse
    // un recouvrement nommer la bonne ligne du bon fichier.
    for (const asmb::Object &obj : objects) {
        const uint16_t base = (uint16_t)lk.sites_.size();
        for (const asmb::Site &s : obj.sites) lk.sites_.push_back(s);
        lk.place(obj, base);
    }
    lk.flushOverlap();   // le dernier chevauchement accumulé doit sortir avant le reste

    for (const auto &kv : lk.spaces_) lk.out.banksWritten.push_back(kv.first);

    if (lk.hi_ > lk.lo_) {
        lk.out.loadAddress = (uint16_t)lk.lo_;
        lk.out.bin.assign((size_t)(lk.hi_ - lk.lo_), 0);
        for (int a = lk.lo_; a < lk.hi_; ++a)
            lk.out.bin[(size_t)(a - lk.lo_)] = lk.spaces_[a / 0x4000].bytes[a % 0x4000];
    }

    // Le point d'entrée : un NOM, résolu ICI. Sans `run`, c'est la première
    // adresse écrite — ce qui reste la seule réponse défendable.
    //
    // Un nom que la table ne porte pas retombe sur `value` sans rien dire de
    // plus : l'assembleur n'écrit un nom que s'il l'a lui-même résolu, et un
    // nom qu'il n'a pas résolu a déjà produit son erreur, à sa ligne. En dire
    // une seconde ici ne ferait que doubler le diagnostic.
    lk.out.runAddress = lk.out.loadAddress;
    for (const asmb::Object &obj : objects) {
        if (!obj.entry.has) continue;
        int run = (int)obj.entry.value;
        auto it = obj.symbols.find(obj.entry.name);
        if (!obj.entry.name.empty() && it != obj.symbols.end()) run = (int)it->second;
        lk.out.runAddress = (uint16_t)run;
        lk.warnRunDisplaced(obj, run);
        break;   // un seul point d'entrée ; B8 dira quoi faire de deux
    }

    lk.out.ok = lk.out.errors.empty();
    return lk.out;
}

Flat flatten(const Image &img) {
    Flat f;
    f.bytes.assign((size_t)kFlatBanks * 0x4000, 0);
    f.covered.assign((size_t)kFlatBanks * 0x4000, 0);
    for (const Block &b : img.blocks) {
        for (size_t k = 0; k < b.bytes.size(); ++k) {
            if (!b.covered[k]) continue;
            const int addr = (b.addr + (int)k) & 0xFFFF;
            const int bank = b.bank < 0 ? ((addr >> 14) & 3) : b.bank;
            if (bank >= kFlatBanks) continue;   // hors de portée d'un dump plat
            const size_t flat = (size_t)bank * 0x4000 + (addr & 0x3FFF);
            f.bytes[flat] = b.bytes[k];
            f.covered[flat] = 1;
        }
    }
    return f;
}

} // namespace link
