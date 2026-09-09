// link.cpp - Le linker (voir link.h)
//
// Le placement de l'étage B tient en une phrase : chaque fragment va où son
// `org` le dit, dans l'ordre où l'assembleur l'a produit. Rejouer cet ordre
// rejoue les écritures dans leur ordre d'origine — c'est ce qui fait qu'un
// recouvrement se diagnostique là où il s'est produit, et pas ailleurs.
#include "link.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>

namespace link {

std::map<std::string, int64_t> switchSymbols(const script::Script &script,
                                             const profile::Profile &profile);

namespace {

// Le nom d'un objet tel qu'un diagnostic doit le citer.
std::string obj_name(const asmb::Object &o) { return o.name; }

std::string hexSize(int64_t v) { char b[32]; snprintf(b, sizeof b, "0x%llX", (unsigned long long)v); return b; }
std::string lower(std::string s) { for (char &c : s) c = (char)std::tolower((unsigned char)c); return s; }

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

// Une SECTION DU LINKAGE : le même nom, dans autant d'unités qu'on veut, ne
// fait qu'une section. C'est ce qui donne au linker le gain que le §11 lui
// attribue — concaténer les sections "ro" de dix fichiers pour remplir au plus
// juste une ROM de 16 K —, et ce qu'aucune unité ne peut décider seule.
//
// Ce qu'elle porte vient de sa PREMIÈRE déclaration, et les suivantes doivent
// s'y accorder : le type, le plafond, et le fait d'être relocalisable ou placée
// par son `org`.
struct Merged {
    std::string name;
    int declaredBy = -1;         // l'unité qui l'a déclarée la première
    int declaredIn = 1;          // dans combien d'unités elle est déclarée
    std::string file;            // la ligne de cette première déclaration
    int line = 0;
    std::string kind;
    bool relocatable = false;
    bool hasMax = false;
    int64_t max = 0;
    // Deux unites se sont contredites sur cette section. Le controle du plafond
    // se tait alors : verifier une somme contre un plafond qu'on vient de
    // declarer indecidable serait tirer au sort une des deux lectures, puis
    // rapporter un depassement sur ce tirage.
    bool disagreed = false;
    // La somme des octets ÉMIS, pour le plafond — et non l'étendue, qui sert au
    // placement. C'est la quantité que l'étape A1 compare au plafond, et la
    // comparer à une autre ici ferait dire deux choses à un même `max`.
    int64_t declaredSize = 0;
};

// Quelle banque un slot désigne, et quel nombre elle porte.
//
// Deux formes partagent la notation `<>`, et elles ne veulent pas dire la même
// chose. `ext<b>` renvoie à des banques RÉELLEMENT DÉCLARÉES — `ext0..ext3` —
// et le paramètre en choisit une. `rom_hi<n>` renvoie à UNE banque déclarée
// paramétriquement, et le paramètre est un numéro que le matériel lui donne, non
// un indice dans une liste.
//
// Le premier cas se reconnaît à ce que la banque concaténée existe ; le second à
// ce que la banque nue existe et se déclare paramétrique. Chercher dans cet
// ordre est ce qui permet aux deux de coexister sans un mot de vocabulaire de
// plus.
const profile::Bank *resolveBank(const profile::Profile &pr, const profile::Slot &sl,
                                 int64_t arg, std::string &name, int64_t &page) {
    auto find = [&](const std::string &n) -> const profile::Bank * {
        for (const profile::Bank &b : pr.banks) if (b.name == n) return &b;
        return nullptr;
    };
    page = 0;
    if (!sl.hasParam) {
        name = sl.bank;
        const profile::Bank *b = find(name);
        if (b && b->hasPage) page = b->page;
        return b;
    }
    const int64_t v = sl.literal ? sl.value : arg;
    name = sl.bank + std::to_string(v);
    if (const profile::Bank *b = find(name)) {
        if (b->hasPage) page = b->page;
        return b;
    }
    if (const profile::Bank *b = find(sl.bank)) {
        if (b->hasPage) { name = sl.bank; page = v; return b; }
    }
    return nullptr;
}

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
    // La base decidee pour chaque section relocalisable, par objet : elle est
    // relue plus tard, quand la table des symboles et le point d'entree se
    // resolvent.
    std::map<int, int> relocBase_;
    std::vector<std::map<int, int>> bases_;
    // L'emplacement de RANGEMENT que le script a décidé pour une section, quand
    // il en a décidé un. Il voyage à côté de la base parce qu'il vient du même
    // calcul : la fenêtre donne l'adresse logique, la configuration donne la
    // banque, et le profil dit sous quel numéro cette banque se range.
    std::map<int, int> relocStore_;
    std::vector<std::map<int, int>> stores_;
    // Les symboles que les objets EXPORTENT, par nom : c'est contre elle que les
    // `EXTERN` se resolvent.
    std::map<std::string, int64_t> exported_;
    std::set<std::string> unresolved_;   // deja signales, pour ne le dire qu'une fois
    // De quel objet vient l'octet pose a chaque adresse : c'est ce qui permet de
    // distinguer une reecriture INTERNE — un idiome — d'un recouvrement entre
    // deux unites assemblees separement, qui n'est jamais voulu.
    std::map<int, std::vector<int>> owner_;
    std::set<long long> clash_;          // paires d'objets deja signalees
    int curObject_ = -1;
    std::vector<std::string> objectNames_;

    std::string objectLabel(int oi) const {
        if (oi < 0 || (size_t)oi >= objectNames_.size() || objectNames_[(size_t)oi].empty())
            return "object " + std::to_string(oi < 0 ? 0 : oi + 1);
        return "'" + objectNames_[(size_t)oi] + "'";
    }
    int baseOf(int section) const {
        if (section < 0) return 0;
        auto it = relocBase_.find(section);
        return it == relocBase_.end() ? 0 : it->second;
    }
    // -1 : aucun script n'a décidé du rangement de cette section, et la banque
    // suit alors l'adresse comme elle l'a toujours fait (ADR 0005).
    int storeOf(int section) const {
        if (section < 0) return -1;
        auto it = relocStore_.find(section);
        return it == relocStore_.end() ? -1 : it->second;
    }

    std::string siteLabel(uint16_t id) const {
        if (id == 0 || id == kUnknownSite || id > sites_.size()) return "site inconnu";
        const asmb::Site &s = sites_[id - 1];
        return s.file + ":" + std::to_string(s.line);
    }

    // Un diagnostic attribue a la LIGNE qui a ecrit l'octet vise : c'est la
    // seule que son auteur peut corriger, et la provenance voyage justement
    // avec les octets pour cela.
    void diagAt(const asmb::Object &obj, const asmb::Fragment &f, int offset, const std::string &msg) {
        const uint16_t site = offset >= 0 && (size_t)offset < f.prov.size() ? f.prov[(size_t)offset] : 0;
        const bool known = site != 0 && site != kUnknownSite && site <= obj.sites.size();
        out.errors.push_back({known ? obj.sites[site - 1].file : std::string(),
                              known ? obj.sites[site - 1].line : 0, msg});
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

    // Deux unités qui déclarent le même nom de section doivent en dire la même
    // chose. Le diagnostic est attribué à la SECONDE déclaration — celle qui
    // diverge, et la seule que son auteur peut changer sans toucher à l'autre
    // unité — et il nomme les deux, sur le modèle exact du refus interne de
    // l'étage A.
    void agree(Merged &m, const asmb::Section &sec, int oi) {
        ++m.declaredIn;
        const std::string what = "section '" + sec.name + "': ";
        const std::string first = objectLabel(m.declaredBy), second = objectLabel(oi);
        auto refuse = [&](const std::string &msg) {
            m.disagreed = true;
            out.errors.push_back({sec.file, sec.line, what + msg});
        };
        // Le type, d'abord : c'est lui qui arme le refus d'écriture en "ro", et
        // le laisser changer d'une unité à l'autre le désarmerait en silence.
        if (!sec.kind.empty() && !m.kind.empty() && sec.kind != m.kind)
            refuse(first + " declares it \"" + lower(m.kind) + "\", " + second +
                   " declares it \"" + lower(sec.kind) + "\" — a section keeps the "
                   "type of its first declaration");
        // Le plafond. Retenir le plus petit serait défendable, et c'est
        // précisément la raison de refuser : personne ne pourrait deviner
        // laquelle des deux lectures a été appliquée.
        // L'ordre du message est toujours PREMIERE puis SECONDE declaration, et
        // jamais « celle qui declare un plafond » puis « celle qui n'en declare
        // pas » : la regle citee parle de la premiere, et un message qui les
        // nomme dans l'autre ordre la rend incomprehensible.
        if (m.hasMax != sec.hasMax)
            refuse(first + (m.hasMax ? " declares a maximum size of " + hexSize(m.max)
                                     : std::string(" declares no maximum size")) + ", " +
                   second + (sec.hasMax ? " declares " + hexSize(sec.max)
                                        : std::string(" declares none")) +
                   " — a section keeps the maximum size of its first declaration");
        else if (m.hasMax && m.max != sec.max)
            refuse(first + " declares a maximum size of " + hexSize(m.max) + ", " +
                   second + " declares " + hexSize(sec.max) +
                   " — a section keeps the maximum size of its first declaration");
        // Relocalisable d'un côté, placée par son `org` de l'autre : la section
        // fusionnée ne peut pas être les deux, et il n'y a pas de lecture par
        // défaut à préférer.
        if (m.relocatable != sec.relocatable)
            refuse((m.relocatable ? first : second) + " lets the linker place it, " +
                   (m.relocatable ? second : first) + " places it with 'org'"
                   " — a section is relocatable in every unit, or in none");
    }

    // Ce que le script a décidé pour une section : son adresse logique, son
    // emplacement de rangement, et la place qui lui reste.
    struct Slotting {
        int64_t base = 0;
        int store = 0;
        int64_t capacity = 0;      // ce que le bloc peut encore porter
        std::string config, window, bank;
        std::string file;
        int line = 0;
    };

    // Une RÉGION de placement : un intervalle d'adresses logiques dans un
    // emplacement de rangement, et de qui il tient.
    //
    // Les trois provenances ne se valent pas, et c'est ce qui décide de qui se
    // compare à qui : deux régions ABSOLUES qui se recouvrent sont l'affaire du
    // contrôle octet par octet, qui sait déjà distinguer une réécriture — un
    // idiome, à l'intérieur d'un fichier — d'un heurt entre deux unités. Cet
    // étage n'ajoute un refus que là où le SCRIPT est en cause.
    struct Region {
        enum Origin { Script, Derived, Absolute } origin = Script;
        int store = 0;
        int64_t lo = 0, hi = 0;      // adresses logiques, `hi` exclus
        std::string what;            // la section, ou le bloc du script
        std::string bank, window, config;
        std::string file;
        int line = 0;
    };
    std::vector<Region> regions_;
    // Les BLOCS que le script a écrits, comparés entre eux et à rien d'autre.
    // Une section vit DANS son bloc : les mêler ferait de tout placement un
    // recouvrement de lui-même. Et deux blocs disjoints ont des sections
    // disjointes, si bien que ce contrôle-ci suffit à couvrir le heurt entre
    // deux sections que le script place toutes les deux.
    std::vector<Region> blocks_;
    // Les emplacements dont la disposition en sections a déjà été refusée : tout
    // ce que leurs octets diront ensuite est du bruit en aval.
    std::set<int> spoiled_;

    const profile::Window *windowNamed(const profile::Profile &pr, const std::string &n) const {
        for (const profile::Window &w : pr.windows) if (w.name == n) return &w;
        return nullptr;
    }
    const profile::Bank *bankNamed(const profile::Profile &pr, const std::string &n) const {
        for (const profile::Bank &b : pr.banks) if (b.name == n) return &b;
        return nullptr;
    }

    // Retrouver l'état qu'une référence de script désigne. L'axe est facultatif
    // quand le nom d'état suffit ; quand il ne suffit pas, c'est-à-dire quand
    // deux axes portent le même nom d'état, le refus le dit plutôt que d'en
    // choisir un — `on` appartient à autant d'axes qu'il y a de recouvrements.
    const profile::State *stateOf(const profile::Profile &pr, const script::ConfigRef &ref,
                                  const std::string &where, int line, std::string &axisName) {
        const profile::State *found = nullptr;
        std::string firstAxis;
        int matches = 0;
        for (const profile::Axis &a : pr.axes) {
            if (!ref.axis.empty() && a.name != ref.axis) continue;
            for (const profile::State &st : a.states)
                if (st.name == ref.state) {
                    if (!found) { found = &st; firstAxis = a.name; }
                    ++matches;
                }
        }
        if (!found) {
            out.errors.push_back({where, line,
                "the script places sections in configuration '" +
                (ref.axis.empty() ? ref.state : ref.axis + "." + ref.state) +
                "', which the profile does not declare"});
            return nullptr;
        }
        if (matches > 1) {
            out.errors.push_back({where, line,
                "configuration '" + ref.state + "' is a state of " +
                std::to_string(matches) + " axes: name the axis, as in "
                "'<axis>." + ref.state + "'"});
            return nullptr;
        }
        axisName = firstAxis;
        return found;
    }

    // Le PLAN : la fenêtre donne l'adresse logique, la configuration donne la
    // banque, et le profil dit sous quel numéro cette banque se range. C'est le
    // renversement du §12.2 — la source ne nomme plus d'emplacement, elle nomme
    // une section.
    std::map<std::string, Slotting> planFrom(const script::Script &sc, const profile::Profile &pr,
                                             const std::map<std::string, int64_t> &sizes) {
        std::map<std::string, Slotting> plan;
        for (const script::ConfigBlock &cb : sc.map) {
            std::string axisName;
            const profile::State *st = stateOf(pr, cb.config, cb.file, cb.line, axisName);
            if (!st) continue;
            const std::string configLabel =
                axisName + "." + cb.config.state +
                (cb.config.hasArg ? "<" + std::to_string(cb.config.arg) + ">" : "");
            // Un qualificatif que rien ne consomme est refusé plutôt qu'ignoré :
            // l'auteur croirait avoir dit quelque chose. Le nombre d'un état
            // paramétrique se dit dans son argument, comme partout ailleurs.
            if (!cb.qualifiers.empty()) {
                out.errors.push_back({cb.file, cb.line,
                    "'" + cb.qualifiers.front().name + "' is not consumed: a "
                    "configuration's number goes in its argument, as in '" +
                    cb.config.state + "<n>'"});
                continue;
            }
            for (const script::Placement &p : cb.placements) {
                const profile::Window *win = windowNamed(pr, p.window);
                if (!win) {
                    out.errors.push_back({p.file, p.line,
                        "'" + p.window + "' is not a WINDOW the profile declares"});
                    continue;
                }
                // Quelle banque cet état fait-il apparaître dans cette fenêtre ?
                // S'il n'en dit rien, il ne concerne pas cette fenêtre, et y
                // placer une section serait supposer une carte que personne n'a
                // décrite.
                const profile::Slot *slot = nullptr;
                for (const profile::Slot &sl : st->slots)
                    if (sl.window == p.window) slot = &sl;
                if (!slot) {
                    out.errors.push_back({p.file, p.line,
                        "configuration '" + configLabel + "' says nothing about window '" +
                        p.window + "': it cannot place a section there"});
                    continue;
                }
                if (slot->hasParam && !slot->literal && !cb.config.hasArg) {
                    out.errors.push_back({p.file, p.line,
                        "configuration '" + configLabel + "' takes a parameter, and the "
                        "script gave none: write '" + cb.config.state + "<n>'"});
                    continue;
                }
                std::string bankName;
                int64_t bankPage = 0;
                const profile::Bank *bk = resolveBank(pr, *slot, cb.config.arg, bankName, bankPage);
                if (!bk) {
                    out.errors.push_back({p.file, p.line,
                        "configuration '" + configLabel + "' puts bank '" + bankName +
                        "' in window '" + p.window + "', and the profile declares no such bank"});
                    continue;
                }
                // La place disponible est la plus petite des trois : la fenêtre,
                // la banque, et le découpage que le script a écrit. Les trois
                // sont des bornes réelles, et retenir la plus petite est la seule
                // réponse qui ne mente pas.
                const int64_t span = win->hi - win->lo + 1;
                int64_t room = span < bk->size ? span : bk->size;
                int64_t origin = win->lo;
                if (p.hasRange) {
                    if (p.offset + p.size > room) {
                        out.errors.push_back({p.file, p.line,
                            "[OFFSET " + hexSize(p.offset) + ", SIZE " + hexSize(p.size) +
                            "] does not fit in bank '" + bankName + "' seen at window '" +
                            p.window + "' (" + hexSize(room) + " bytes)"});
                        continue;
                    }
                    origin += p.offset;
                    room = p.size;
                }
                Region reg;
                reg.origin = Region::Script;
                reg.store = (int)bk->store;
                reg.lo = origin;
                reg.hi = origin + room;
                reg.what = p.hasRange
                    ? "the block at [OFFSET " + hexSize(p.offset) + ", SIZE " + hexSize(p.size) + "]"
                    : "window '" + p.window + "'";
                reg.bank = bankName;
                reg.window = p.window;
                reg.config = configLabel;
                reg.file = p.file;
                reg.line = p.line;
                blocks_.push_back(reg);

                int64_t used = 0;
                for (const std::string &name : p.sections) {
                    auto prev = plan.find(name);
                    if (prev != plan.end()) {
                        out.errors.push_back({p.file, p.line,
                            "section '" + name + "' is placed twice: already in '" +
                            prev->second.config + "', window '" + prev->second.window + "'"});
                        continue;
                    }
                    auto sz = sizes.find(name);
                    const int64_t need = sz == sizes.end() ? 0 : sz->second;
                    if (used + need > room) {
                        out.errors.push_back({p.file, p.line,
                            "section '" + name + "' overflows bank '" + bankName +
                            "' at window '" + p.window + "' by " +
                            hexSize(used + need - room) + " bytes (" + hexSize(room) +
                            " available, " + hexSize(used + need) + " asked for)"});
                        continue;
                    }
                    Slotting s;
                    s.base = origin + used;
                    s.store = (int)bk->store;
                    s.capacity = room - used;
                    s.config = configLabel;
                    s.window = p.window;
                    s.bank = bankName;
                    s.file = p.file;
                    s.line = p.line;
                    plan[name] = std::move(s);
                    used += need;
                }
                // LE MOU, chiffré. C'est le nombre dont l'auteur a besoin pour
                // arbitrer, et que personne ne calcule à la main (§11). Il n'est
                // dit que là où le script a placé : la banque d'une source qui
                // n'écrit pas de carte n'a pas de budget, et le crier partout
                // ferait du bruit sur l'immense majorité des sources.
                if (used < room) {
                    char at[24];
                    snprintf(at, sizeof at, "&%04X", (unsigned)((origin + used) & 0xFFFF));
                    out.prints.push_back({p.file, p.line,
                        hexSize(room - used) + " bytes unused at " + at + " in bank '" +
                        bankName + "'"});
                }
            }
        }
        return plan;
    }

    // Les symboles de commutation, offerts aux `EXTERN`. Le calcul lui-même est
    // une fonction PURE, hors de cette structure, parce que le CLI l'appelle
    // AVANT d'assembler : c'est ce qui donne à ces symboles l'arithmétique que
    // le §12.2 emploie. Les deux chemins lisent donc la même valeur.
    void offerSwitchSymbols(const script::Script &sc, const profile::Profile &pr) {
        for (const auto &kv : switchSymbols(sc, pr)) exported_[kv.first] = kv.second;
    }

    // Deux régions qui se disputent les mêmes octets d'un même emplacement.
    //
    // C'est le refus que seul un placement CALCULÉ peut prononcer, et il nomme
    // des SECTIONS là où le contrôle octet par octet nomme des lignes : personne
    // ne réécrit une section avec une autre, donc il n'y a pas d'idiome à
    // épargner ici.
    void refuseOverlap(const Region &blamed, const Region &other, int64_t lo, int64_t hi) {
        char range[40];
        snprintf(range, sizeof range, "&%04X-&%04X",
                 (unsigned)(lo & 0xFFFF), (unsigned)((hi - 1) & 0xFFFF));
        out.errors.push_back({blamed.file, blamed.line,
            blamed.what + " and " + other.what + " overlap at " + range +
            " in bank '" + (blamed.bank.empty() ? other.bank : blamed.bank) + "'"});
        spoiled_.insert(blamed.store);
    }

    void checkOverlaps() {
        auto meet = [](const Region &x, const Region &y, int64_t &lo, int64_t &hi) {
            if (x.store != y.store) return false;
            if (x.hi <= y.lo || y.hi <= x.lo) return false;
            lo = x.lo > y.lo ? x.lo : y.lo;
            hi = x.hi < y.hi ? x.hi : y.hi;
            return true;
        };
        // Les blocs du script, entre eux. C'est ce qui refuse deux découpages
        // `[OFFSET, SIZE]` qui se chevauchent dans une banque, et aussi deux
        // configurations qui donnent la même banque à la même fenêtre.
        int64_t lo = 0, hi = 0;
        for (size_t a = 0; a < blocks_.size(); ++a)
            for (size_t b = a + 1; b < blocks_.size(); ++b)
                if (meet(blocks_[a], blocks_[b], lo, hi))
                    refuseOverlap(blocks_[b], blocks_[a], lo, hi);
        // Puis les grilles superposées. Deux fenêtres d'un MÊME état peuvent se
        // recouvrir dans l'espace adressable tout en désignant des banques
        // différentes : sur une machine à mapper, une grille de 8 K redécoupe une
        // page de 16 K, et les deux sont actives ensemble. Les deux sections se
        // croient alors à la même adresse.
        //
        // À l'intérieur d'un état, la carte est FIXE : le recouvrement se décide
        // sans rien savoir de ce que le programme exécute. C'est ce qui distingue
        // ce contrôle de la co-visibilité entre états, qui est l'affaire de C2.
        for (size_t a = 0; a < blocks_.size(); ++a)
            for (size_t b = a + 1; b < blocks_.size(); ++b) {
                const Region &x = blocks_[a], &y = blocks_[b];
                if (x.store == y.store) continue;      // déjà dit, s'il y avait à dire
                if (x.config != y.config) continue;    // deux états : c'est C2
                if (x.hi <= y.lo || y.hi <= x.lo) continue;
                const int64_t l = x.lo > y.lo ? x.lo : y.lo;
                const int64_t h = x.hi < y.hi ? x.hi : y.hi;
                char range[40];
                snprintf(range, sizeof range, "&%04X-&%04X",
                         (unsigned)(l & 0xFFFF), (unsigned)((h - 1) & 0xFFFF));
                out.errors.push_back({y.file, y.line,
                    "windows '" + x.window + "' and '" + y.window +
                    "' overlap at " + range + " in configuration '" + y.config +
                    "': banks '" + x.bank + "' and '" + y.bank +
                    "' are both visible there, and two sections cannot share the address"});
                spoiled_.insert(x.store);
                spoiled_.insert(y.store);
            }

        // Puis les sections, mais SEULEMENT là où le script rencontre autre chose
        // que lui-même. Deux sections que le script place toutes les deux sont
        // déjà couvertes par leurs blocs ; deux régions absolues sont l'affaire
        // du contrôle octet par octet, qui sait y distinguer un idiome d'un
        // heurt.
        for (size_t a = 0; a < regions_.size(); ++a)
            for (size_t b = a + 1; b < regions_.size(); ++b) {
                const Region &x = regions_[a], &y = regions_[b];
                const bool one = (x.origin == Region::Script) != (y.origin == Region::Script);
                if (!one) continue;
                if (!meet(x, y, lo, hi)) continue;
                const Region &blamed = y.origin == Region::Script ? y : x;
                refuseOverlap(blamed, &blamed == &y ? x : y, lo, hi);
            }
    }

    // Où poser les sections que personne n'a placées.
    //
    // Le même nom dans N objets ne fait qu'UNE section : c'est ce qui permet au
    // linker de remplir une ROM avec les sections "ro" de dix fichiers (§11), et
    // sans quoi chaque unité recevrait sa propre base. Sa base est décidée une
    // fois ; chaque objet y range sa contribution à la suite de celles des
    // objets qui le précèdent.
    //
    // En C1.0 l'endroit reste trivial et assumé : les sections se suivent, dans
    // l'ordre de leur PREMIÈRE déclaration, après le dernier octet absolu de
    // TOUS les objets. C1.4 remplacera ce calcul — fenêtres, banques,
    // configurations — sans toucher au reste.
    //
    // Les identifiants de section sont LOCAUX à leur objet : c'est pour cela
    // qu'il y a une table de bases par objet, et non une seule.
    // `sc` est la carte qui PLACE — celle du script, augmentée de ce que les
    // sections portent. `declared` est le script tel que l'auteur l'a écrit :
    // les symboles de commutation s'en tiennent à lui, et le commentaire de
    // `offerSwitchSymbols` dit pourquoi.
    std::vector<std::map<int, int>> placeRelocSections(const std::vector<asmb::Object> &objects,
                                                       const script::Script &sc,
                                                       const script::Script &declared,
                                                       const profile::Profile &pr) {
        int cursor = 0;
        for (const asmb::Object &obj : objects)
            for (const asmb::Fragment &f : obj.fragments)
                if (f.relocSection < 0 && !f.bytes.empty())
                    cursor = std::max(cursor, (f.addr + (int)f.bytes.size()) & 0xFFFF);

        // 1. Les sections, fusionnées par nom. L'ordre est celui de la première
        // déclaration, les objets parcourus dans l'ordre où on les a donnés.
        std::vector<Merged> merged;
        std::map<std::string, size_t> byName;
        for (size_t oi = 0; oi < objects.size(); ++oi) {
            for (const asmb::Section &sec : objects[oi].sections) {
                auto it = byName.find(sec.name);
                if (it == byName.end()) {
                    Merged m;
                    m.name = sec.name;
                    m.declaredBy = (int)oi;
                    m.file = sec.file;
                    m.line = sec.line;
                    m.kind = sec.kind;
                    m.relocatable = sec.relocatable;
                    m.hasMax = sec.hasMax;
                    m.max = sec.max;
                    m.declaredSize = sec.size;
                    byName[sec.name] = merged.size();
                    merged.push_back(std::move(m));
                    continue;
                }
                merged[it->second].declaredSize += sec.size;
                agree(merged[it->second], sec, (int)oi);
            }
        }

        // 2. Ce que chaque objet met dans chaque section : l'ÉTENDUE de ses
        // fragments, et non le nombre d'octets émis. Un `ds` réserve de la place
        // sans l'écrire, et la contribution suivante doit commencer après.
        std::vector<std::map<size_t, int>> extent(objects.size());
        for (size_t oi = 0; oi < objects.size(); ++oi) {
            std::map<int, size_t> local;   // id local -> section fusionnée
            for (const asmb::Section &sec : objects[oi].sections)
                if (sec.id >= 0 && byName.count(sec.name)) local[sec.id] = byName[sec.name];
            for (const asmb::Fragment &f : objects[oi].fragments) {
                if (f.relocSection < 0) continue;
                auto n = local.find(f.relocSection);
                if (n == local.end()) continue;
                int &e = extent[oi][n->second];
                e = std::max(e, f.addr + (int)f.bytes.size());
            }
            // La PLACE DEMANDÉE compte aussi, et pas seulement les octets posés.
            // Une section `"uninit"` n'émet rien : elle n'a aucun fragment, et
            // son étendue serait nulle — le linker ne lui donnerait pas d'adresse
            // et son label vaudrait zéro. Un `ds` en fin de section relocalisable
            // pose le même problème : les octets réservés sont hors du fragment,
            // et la section suivante viendrait s'y poser.
            //
            // `Section::size` est exactement cette place — « une place RÉSERVÉE
            // compte comme une place écrite : c'est la seule information qu'une
            // section "uninit" donne au linker ».
            for (const asmb::Section &sec : objects[oi].sections) {
                if (!sec.relocatable || sec.id < 0 || !byName.count(sec.name)) continue;
                int &e = extent[oi][byName[sec.name]];
                e = std::max(e, (int)sec.size);
            }
        }

        // 3. Ce que le script place PAR CALCUL. L'étendue totale de chaque nom
        // doit être connue avant : c'est elle qui décide de ce qui rentre, et
        // c'est pour cela que ce temps-ci vient après le second.
        std::map<std::string, int64_t> total;
        for (size_t mi = 0; mi < merged.size(); ++mi) {
            int64_t t = 0;
            for (size_t oi = 0; oi < objects.size(); ++oi) {
                auto e = extent[oi].find(mi);
                if (e != extent[oi].end()) t += e->second;
            }
            total[merged[mi].name] = t;
        }
        std::map<std::string, Slotting> plan;
        if (!sc.map.empty()) {
            plan = planFrom(sc, pr, total);
            // Les symboles de commutation sont offerts ICI, dès que le placement
            // est décidé : un `EXTERN` sur `__val_...` doit se résoudre au même
            // temps que les autres, et il ne peut pas l'être avant que la
            // configuration de chaque section soit connue.
            // LES CLÉS PAR SECTION S'EN TIENNENT AU SCRIPT, et c'est pourquoi
            // c'est `declared` qui est passé ici. `switchSymbols` tourne aussi
            // AVANT d'assembler, où le placement du source n'est pas lisible :
            // offrir `__val_ram_gfx1` ici et pas là ferait d'un même nom deux
            // langages selon la façon dont on compile. Un source qui se place
            // lui-même nomme sa configuration — `__val_ram_ext_w1_1` —, et ce
            // nom-là vaut par les deux chemins.
            offerSwitchSymbols(declared, pr);
            // `__off_<section>` : son offset DANS sa banque. Celui-là dépend du
            // placement — pour un loader, ou pour une recopie (§12.3) — et il ne
            // peut donc pas être une constante calculée avant d'assembler. C'est
            // la seule différence de nature entre les deux familles, et elle est
            // dans la fenêtre : l'offset est une adresse moins une base.
            for (const auto &kv : plan)
                exported_["__off_" + kv.first] = kv.second.base & 0x3FFF;
        }
        // Un script qui place une section qu'aucune unité ne porte, ou qui en
        // place une que son `org` a déjà placée : deux fautes que seul cet
        // endroit voit, parce que lui seul a les deux côtés sous les yeux.
        for (const auto &kv : plan) {
            const Merged *m = nullptr;
            for (const Merged &mm : merged) if (mm.name == kv.first) m = &mm;
            if (!m) {
                out.errors.push_back({kv.second.file, kv.second.line,
                    "the script places section '" + kv.first + "', which no object declares"});
                continue;
            }
            if (!m->relocatable)
                out.errors.push_back({kv.second.file, kv.second.line,
                    "the script places section '" + kv.first + "', but it carries an "
                    "'org': a section is placed by its org, or by the script, never both"});
        }

        // 4. Les bases. Une section à la fois, et à l'intérieur, un objet à la
        // fois : c'est cet ordre-là qui met bout à bout les contributions d'un
        // même nom, au lieu de les disperser objet par objet.
        //
        // Une section que le script ne nomme pas suit le PLACEMENT DÉRIVABLE du
        // §9 — à la suite, après le dernier octet absolu. Le cas simple ne paie
        // ni en syntaxe, ni en fichier, ni en argument de ligne de commande.
        std::vector<std::map<int, int>> all(objects.size());
        stores_.assign(objects.size(), std::map<int, int>());
        for (size_t mi = 0; mi < merged.size(); ++mi) {
            if (!merged[mi].relocatable) continue;
            auto placed = plan.find(merged[mi].name);
            int64_t where = placed == plan.end() ? cursor : placed->second.base;
            for (size_t oi = 0; oi < objects.size(); ++oi) {
                auto e = extent[oi].find(mi);
                if (e == extent[oi].end() || e->second <= 0) continue;
                int id = -1;
                for (const asmb::Section &sec : objects[oi].sections)
                    if (sec.name == merged[mi].name) { id = sec.id; break; }
                if (id < 0) continue;
                all[oi][id] = (int)where;
                if (placed != plan.end()) stores_[oi][id] = placed->second.store;
                where += e->second;
            }
            if (placed == plan.end()) cursor = (int)where;
            // La région de cette section, pour le contrôle de recouvrement. Une
            // section que le script ne place pas en est une aussi : c'est
            // justement le heurt entre les deux qui n'est visible que d'ici.
            const int64_t start = placed == plan.end() ? cursor - total[merged[mi].name]
                                                       : placed->second.base;
            if (total[merged[mi].name] > 0) {
                Region reg;
                reg.origin = placed == plan.end() ? Region::Derived : Region::Script;
                reg.store = placed == plan.end() ? (int)((start >> 14) & 3)
                                                 : placed->second.store;
                reg.lo = start;
                reg.hi = start + total[merged[mi].name];
                reg.what = "section '" + merged[mi].name + "'";
                reg.bank = placed == plan.end() ? std::string() : placed->second.bank;
                reg.file = placed == plan.end() ? merged[mi].file : placed->second.file;
                reg.line = placed == plan.end() ? merged[mi].line : placed->second.line;
                regions_.push_back(std::move(reg));
            }
        }

        // Les fragments ABSOLUS, pour que le heurt entre un `org` et un
        // placement calculé se dise en nommant les deux. Un fragment et non une
        // section : une section à plusieurs `org` n'occupe pas un intervalle.
        for (const asmb::Object &obj : objects)
            for (const asmb::Fragment &f : obj.fragments) {
                if (f.relocSection >= 0 || f.bytes.empty()) continue;
                Region reg;
                reg.origin = Region::Absolute;
                reg.store = f.bank < 0 ? (int)((f.addr >> 14) & 3) : f.bank;
                reg.lo = f.addr;
                reg.hi = f.addr + (int64_t)f.bytes.size();
                reg.what = f.section.empty() ? std::string("bytes outside any section")
                                             : "section '" + f.section + "', placed by its 'org'";
                reg.file = obj.name;
                regions_.push_back(std::move(reg));
            }

        checkOverlaps();

        // 4. Le plafond, sur la SOMME. Chaque unité tient, leur somme non : c'est
        // le refus que l'assembleur ne pouvait pas prononcer, puisqu'il ne voit
        // qu'une unité. Attribué à la ligne qui PORTE le plafond, seule ligne que
        // son auteur peut corriger (§4.1, étape A1).
        //
        // Une seule unité déclarante ne passe pas ici : l'assembleur l'a déjà
        // refusée, et deux diagnostics pour un seul fait en valent zéro.
        for (const Merged &m : merged) {
            if (m.declaredIn < 2 || m.disagreed || !m.hasMax || m.declaredSize <= m.max) continue;
            out.errors.push_back({m.file, m.line,
                "section '" + m.name + "' exceeds maximum declared size (" +
                hexSize(m.declaredSize) + " > " + hexSize(m.max) + " bytes), summed over " +
                std::to_string(m.declaredIn) + " units"});
        }
        return all;
    }

    // Résoudre les relocalisations : écrire, dans les octets du fragment, la
    // valeur que seule la connaissance des bases permettait de calculer. C'est
    // fait AVANT le placement — les octets partent ensuite tels quels, et ce
    // qu'un dump montre est ce que la machine exécutera.
    //
    // Le linker ÉCRIT la valeur, il ne l'additionne pas à ce qui s'y trouve :
    // un objet dont l'addend est faux se lit alors à l'œil.
    void resolve(std::vector<asmb::Fragment> &frags, const asmb::Object &obj) {
        for (const asmb::Reloc &r : obj.relocs) {
            if (r.frag < 0 || (size_t)r.frag >= frags.size()) continue;
            asmb::Fragment &f = frags[(size_t)r.frag];
            int64_t target = baseOf(r.section) + r.addend;
            // Un nom declare EXTERN : c'est un autre objet qui doit le definir,
            // et un `PUBLIC` qui manque doit se dire ici plutot que de laisser
            // des zeros passer pour une adresse.
            if (!r.symbol.empty()) {
                auto it = exported_.find(r.symbol);
                if (it == exported_.end()) {
                    // Une fois par symbole : un nom de bibliotheque manquant est
                    // employe cinquante fois, et cinquante lignes identiques
                    // noieraient les autres diagnostics.
                    if (unresolved_.insert(r.symbol).second)
                        diagAt(obj, f, r.offset,
                               "unresolved EXTERN symbol '" + r.symbol + "': " +
                               objectLabel(curObject_) + " asks for it, no object exports it");
                    continue;
                }
                target = it->second + r.addend;
            }
            auto put = [&](size_t k, uint8_t b) {
                if (k < f.bytes.size()) f.bytes[k] = b;
            };
            switch (r.kind) {
                case asmb::Reloc::Abs16:
                    put((size_t)r.offset, (uint8_t)(target & 0xFF));
                    put((size_t)r.offset + 1, (uint8_t)((target >> 8) & 0xFF));
                    break;
                case asmb::Reloc::High8: put((size_t)r.offset, (uint8_t)((target >> 8) & 0xFF)); break;
                case asmb::Reloc::BankOf: {
                    // L'EMPLACEMENT DE RANGEMENT de la section visée. C'est la
                    // seule relocalisation dont la valeur ne se déduit d'aucune
                    // adresse : elle vient de ce que le script a décidé, et elle
                    // vaut la dérivation historique quand aucun script n'a rien
                    // dit (ADR 0005).
                    const int store = storeOf(r.section);
                    const int bank = store >= 0 ? store : (int)((target >> 14) & 3);
                    put((size_t)r.offset, (uint8_t)(bank & 0xFF));
                    break;
                }
                case asmb::Reloc::Low8:  put((size_t)r.offset, (uint8_t)(target & 0xFF)); break;
                case asmb::Reloc::Rel8: {
                    // Le déplacement se compte depuis l'octet SUIVANT celui qui
                    // le porte. Ici, et nulle part ailleurs, les deux adresses
                    // sont connues — c'est donc ici que la portée se refuse
                    // quand elle traverse deux sections.
                    const int64_t from = f.addr + baseOf(f.relocSection) + r.offset + 1;
                    const int64_t d = target - from;
                    if (d < -128 || d > 127) {
                        diagAt(obj, f, r.offset,
                               "relative jump out of range (-128..127): the target is "
                               "in another section, and the distance is only known here");
                        break;
                    }
                    put((size_t)r.offset, (uint8_t)(d & 0xFF));
                    break;
                }
            }
        }
    }

    // L'adresse definitive d'un symbole. Elle n'existe qu'ICI : dans une section
    // relocalisable, l'assembleur n'en connaissait que l'offset.
    int64_t symbolAddress(const asmb::Object &obj, const asmb::Symbol &s) const {
        if (s.isConst || s.frag < 0 || (size_t)s.frag >= obj.fragments.size()) return s.value;
        const asmb::Fragment &f = obj.fragments[(size_t)s.frag];
        if (f.relocSection < 0) return s.value;
        return (f.logical + baseOf(f.relocSection) + s.offset) & 0xFFFF;
    }

    void place(const asmb::Object &obj, uint16_t siteBase) {
        for (const asmb::Fragment &f : obj.fragments) {
            const int fragAddr = f.addr + baseOf(f.relocSection);
            Block blk;
            bool open = false;
            // Le rangement que le script a décidé l'emporte sur la dérivation
            // par l'adresse : c'est exactement ce que C1 apporte, et sans lui
            // `audio` vu en `&4000` se rangerait en banque 1 comme `main`.
            const int forced = storeOf(f.relocSection);
            for (size_t k = 0; k < f.bytes.size(); ++k) {
                const int addr = (fragAddr + (int)k) & 0xFFFF;
                const int bank = forced >= 0 ? forced : bankOf(f, addr);
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
                std::vector<int> &own = owner_[bank];
                if (own.empty()) own.assign(0x4000, -1);
                // Un emplacement dont la disposition en sections a déjà été
                // refusée ne dira plus rien de ses octets : deux diagnostics
                // pour un seul fait en valent zéro.
                if (spoiled_.count(bank)) { sp.bytes[off] = f.bytes[k]; continue; }
                const int prevOwner = own[(size_t)off];
                own[(size_t)off] = curObject_;
                if (prevOwner >= 0 && prevOwner != curObject_) {
                    // Deux objets qui se disputent une adresse : c'est un REFUS,
                    // pas un avertissement. À l'intérieur d'un fichier, réécrire
                    // est un idiome que l'auteur voit ; entre deux unités
                    // assemblées séparément, personne ne l'a voulu et personne ne
                    // le verrait.
                    //
                    // Une fois par PAIRE d'objets : deux unités qui se recouvrent
                    // le font sur toute une plage, et une ligne par octet
                    // noierait tout le reste. La première adresse suffit à aller
                    // voir. Et le refus REMPLACE l'avertissement de
                    // chevauchement : deux diagnostics pour un seul fait en
                    // valent zéro.
                    const long long pair = (long long)std::min(prevOwner, curObject_) * 4096 +
                                           std::max(prevOwner, curObject_);
                    if (clash_.insert(pair).second) {
                        char at[16];
                        snprintf(at, sizeof at, "&%04X", addr);
                        flushOverlap();
                        diagAt(obj, f, (int)k,
                               std::string("two objects write to ") + at + ": " +
                               objectLabel(prevOwner) + " and " + objectLabel(curObject_) +
                               " — separately assembled units cannot share an address");
                    }
                    sp.prov[off] = site;
                    sp.bytes[off] = f.bytes[k];
                    continue;
                }
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
            const int base = baseOf(f.relocSection);
            for (size_t k = 0; k < f.bytes.size(); ++k) {
                if (f.prov[k] == 0) continue;
                if (((f.logical + base + (int)k) & 0xFFFF) != r) continue;
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


// --- Les symboles de commutation (§12.3), calcules sans une adresse ---------
// Retrouver l'etat qu'une reference de script designe, SANS diagnostic : cette
// fonction sert aussi au CLI, avant qu'aucun objet existe, et un refus s'y dirait
// deux fois.
const profile::State *findState(const profile::Profile &pr, const script::ConfigRef &ref,
                                std::string &axisName) {
    const profile::State *found = nullptr;
    int matches = 0;
    for (const profile::Axis &a : pr.axes) {
        if (!ref.axis.empty() && a.name != ref.axis) continue;
        for (const profile::State &st : a.states)
            if (st.name == ref.state) {
                if (!found) { found = &st; axisName = a.name; }
                ++matches;
            }
    }
    return matches == 1 ? found : nullptr;
}

    // --- Les symboles de commutation (§12.3) --------------------------------
    // Évaluer une expression de `SELECT`. Les noms qu'elle porte — `CODE`,
    // `PAGE`, et le paramètre de l'état — ne valent quelque chose qu'ICI : au
    // moment où une section est placée, donc où l'état, son argument et sa
    // banque sont tous connus. C'est la raison pour laquelle l'analyseur du
    // profil en rend un ARBRE et non un nombre.
bool evalSel(const profile::Expr &e, const std::map<std::string, int64_t> &bind,
             int64_t &v, std::string &missing) {
        switch (e.kind) {
            case profile::Expr::Num: v = e.num; return true;
            case profile::Expr::Name: {
                auto it = bind.find(e.name);
                if (it == bind.end()) { missing = e.name; return false; }
                v = it->second;
                return true;
            }
            case profile::Expr::Unary: {
                int64_t a = 0;
                if (e.args.size() != 1 || !evalSel(e.args[0], bind, a, missing)) return false;
                v = e.op == "~" ? ~a : -a;
                return true;
            }
            case profile::Expr::Binary: {
                int64_t a = 0, b = 0;
                if (e.args.size() != 2) return false;
                if (!evalSel(e.args[0], bind, a, missing)) return false;
                if (!evalSel(e.args[1], bind, b, missing)) return false;
                if (e.op == "|") v = a | b;
                else if (e.op == "&") v = a & b;
                else if (e.op == "^") v = a ^ b;
                else if (e.op == "<<") v = a << b;
                else if (e.op == ">>") v = a >> b;
                else if (e.op == "+") v = a + b;
                else if (e.op == "-") v = a - b;
                else if (e.op == "*") v = a * b;
                else if (e.op == "/") { if (b == 0) return false; v = a / b; }
                else return false;
                return true;
            }
        }
        return false;
    }

    // Les symboles que le linker OFFRE, et qu'aucun objet n'exporte : le triplet
    // par axe du §12.3.
    //
    // Un symbole ne peut pas être « l'octet » du port : sur une machine où un
    // même port porte quatre axes, y sortir la seule valeur de l'axe de
    // pagination écraserait les trois autres, en silence. D'où un port, une
    // valeur BORNÉE AUX BITS DE L'AXE, et le masque de ces bits.
std::map<std::string, int64_t> compute(const script::Script &sc,
                                       const profile::Profile &pr) {
    std::map<std::string, int64_t> out, fromProfile;
    std::set<std::string> ambiguous, profileAmbiguous;
    // Les deux chemins offrent dans DEUX paniers, et le second ne peut pas
    // defaire le premier : voir la fusion, en bas de cette fonction.
    std::map<std::string, int64_t> *dest = &out;
    std::set<std::string> *destAmbiguous = &ambiguous;
    // Offrir un symbole. Une seconde offre du même nom avec une AUTRE valeur le
    // retire : un symbole qui vaudrait deux choses selon l'ordre de lecture est
    // pire qu'un symbole absent, et l'absence a déjà son diagnostic — celui de
    // l'`EXTERN` non résolu.
    auto offer = [&](const std::string &name, int64_t v) {
        if (destAmbiguous->count(name)) return;
        auto it = dest->find(name);
        if (it == dest->end()) { (*dest)[name] = v; return; }
        if (it->second != v) { destAmbiguous->insert(name); dest->erase(it); }
    };
    // Le SELECT d'un axe : les écritures qui l'atteignent.
    auto selFor = [&](const std::string &axisName) -> const profile::Select * {
        const profile::Select *sel = nullptr;
        for (const profile::Select &sl : pr.selects)
            for (const std::string &ax : sl.axes)
                if (ax == axisName) sel = &sl;
        return sel;
    };

    // LE CALCUL D'UNE CARTE : un axe, un état, son argument s'il en a un, et
    // UNE fenêtre. Il ne dépend d'aucune adresse — c'est ce qui permet de
    // l'appeler par DEUX chemins sans le dupliquer : les blocs du script, qui
    // savent quelle section va où, et le profil seul, qui n'en sait rien mais
    // connaît toutes les cartes atteignables. Une fonction, donc une valeur :
    // les deux chemins ne peuvent pas diverger.
    auto emit = [&](const std::string &axisName, const profile::State &st,
                    bool hasArg, int64_t arg, const profile::Slot *slot,
                    const std::vector<std::string> &keys) {
        const profile::Select *sel = selFor(axisName);
        if (!sel || sel->writes.empty()) return;
        const profile::Write &w = sel->writes.front();
        // Un axe peut demander DEUX ecritures, et la seconde porte alors le
        // numero que le §12.3 nomme `__romnum_` : la premiere dit « cette
        // banque-la apparait », la seconde dit LAQUELLE.
        const profile::Write *second = sel->writes.size() > 1 ? &sel->writes[1] : nullptr;

        std::string bankName;
        int64_t bankPage = 0;
        if (slot) {
            // UNE CARTE DONT LA BANQUE N'EXISTE PAS N'EST PAS UNE CARTE. Le
            // placement la refuse déjà — « the profile declares no such bank »
            // — et lui donner `PAGE = 0` pour continuer rendrait un nombre
            // vraisemblable que rien n'a décidé, sur un chemin où plus personne
            // ne le contredira.
            if (!resolveBank(pr, *slot, arg, bankName, bankPage)) return;
        }
        // Les liaisons : le paramètre de l'état, puis `CODE` qui s'en
        // déduit, puis `PAGE`, qui est le nombre que porte la banque —
        // l'indice d'une banque déclarée en plage, ou le numéro que
        // l'argument donne à une banque paramétrique. Un état qui ne mappe
        // AUCUNE fenêtre — le `off` d'un recouvrement — n'a pas de banque, et
        // sa valeur ne tient qu'à son `CODE`.
        std::map<std::string, int64_t> bind;
        if (st.hasParam && hasArg) bind[st.param] = arg;
        bind["PAGE"] = bankPage;
        std::string missing;
        int64_t code = 0;
        if (st.hasCode && !evalSel(st.code, bind, code, missing)) return;
        bind["CODE"] = code;

        int64_t port = 0, val = 0, mask = 0;
        if (!evalSel(w.port, bind, port, missing)) return;
        if (!evalSel(w.value, bind, val, missing)) return;
        const bool hasMask = w.hasMask && evalSel(w.mask, bind, mask, missing);
        // La valeur est BORNÉE AUX BITS DE L'AXE dès qu'un masque les
        // nomme : c'est ce qui permet au source d'écrire
        // `(état & ~masque) | valeur` sans toucher aux axes voisins.
        if (hasMask) val &= mask;

        int64_t port2 = 0, val2 = 0, mask2 = 0;
        bool has2 = false, has2Mask = false;
        if (second) {
            has2 = evalSel(second->port, bind, port2, missing) &&
                   evalSel(second->value, bind, val2, missing);
            has2Mask = has2 && second->hasMask &&
                       evalSel(second->mask, bind, mask2, missing);
            if (has2Mask) val2 &= mask2;
        }
        for (const std::string &k : keys) {
            offer("__port_" + axisName + "_" + k, port);
            offer("__val_" + axisName + "_" + k, val);
            if (has2) {
                offer("__port2_" + axisName + "_" + k, port2);
                offer("__romnum_" + axisName + "_" + k, val2);
            }
        }
        if (hasMask) offer("__mask_" + axisName, mask);
        if (has2Mask) offer("__mask2_" + axisName, mask2);
    };

    // --- Premier chemin : LES BLOCS DU SCRIPT -------------------------------
        // Combien de fois chaque état est nommé : la graphie « par état » n'est
        // offerte que si elle désigne une seule chose. Un état paramétrique
        // nommé deux fois avec deux arguments n'en désigne pas une.
        std::map<std::string, int> named;
        for (const script::ConfigBlock &cb : sc.map) ++named[cb.config.state];

        for (const script::ConfigBlock &cb : sc.map) {
            std::string axisName;
            const profile::State *st = findState(pr, cb.config, axisName);
            if (!st) continue;
            for (const script::Placement &p : cb.placements) {
                const profile::Slot *slot = nullptr;
                for (const profile::Slot &sl : st->slots)
                    if (sl.window == p.window) slot = &sl;
                if (!slot) continue;
                std::vector<std::string> keys(p.sections.begin(), p.sections.end());
                if (named[cb.config.state] == 1) keys.push_back(cb.config.state);
                emit(axisName, *st, cb.config.hasArg, cb.config.arg, slot, keys);
            }
        }

    // --- Second chemin : LE PROFIL SEUL -------------------------------------
    // `switchSymbols` est appelée AVANT d'assembler : elle ne peut pas savoir
    // quelles cartes le source nomme, donc ses clés se dérivent du profil seul.
    //
    // La clé est L'ÉTAT, plus son argument s'il en a un — `ext_w1_1`. Nommer la
    // BANQUE ne déterminerait rien : une même banque peut être amenée dans une
    // même fenêtre par deux états, avec deux valeurs de commutation, et la règle
    // de retrait effacerait alors le symbole. Un état et son argument désignent
    // une carte, et une carte donne une valeur.
    //
    // C'est la graphie « par état » du premier chemin prolongée d'un argument,
    // non une quatrième forme : sur un état sans paramètre, les deux chemins
    // offrent le MÊME nom avec la MÊME valeur, et la règle de retrait ne s'en
    // émeut pas.
    dest = &fromProfile;
    destAmbiguous = &profileAmbiguous;
    for (const profile::Axis &ax : pr.axes) {
        for (const profile::State &st : ax.states) {
            if (!st.hasParam) {
                // UN ÉTAT QUI NE MAPPE AUCUNE FENÊTRE EN EST UN QUAND MÊME :
                // le `off` d'un axe de recouvrement est ce qui REND la RAM, et
                // c'est une valeur qu'aucun script ne pourra jamais offrir —
                // il n'y a rien à y placer. Sans elle, le masque de l'axe
                // n'existerait pas non plus, et le §12.3 en a besoin pour
                // toucher un bit sans écraser les axes voisins du même port.
                if (st.slots.empty()) { emit(ax.name, st, false, 0, nullptr, {st.name}); continue; }
                for (const profile::Slot &sl : st.slots)
                    emit(ax.name, st, false, 0, &sl, {st.name});
                continue;
            }
            // LES VALEURS DU PARAMÈTRE SONT CELLES QUE LES BANQUES DÉCLARÉES
            // BORNENT. `ext<b>` avec `BANK ext0..ext3` en donne quatre ; un
            // `rom_hi<n>` déclaré paramétriquement n'en borne aucune — son
            // numéro vient du matériel, et l'énumérer demanderait d'inventer une
            // borne que le profil ne dit pas. Un paramètre non borné n'offre
            // donc rien, et c'est un manque nommé, pas un oubli.
            //
            // ET C'EST UNE INTERSECTION, non une réunion : une valeur que l'une
            // des fenêtres de l'état ne sait pas honorer ne désigne pas une
            // carte à moitié atteignable, elle n'en désigne aucune.
            std::set<int64_t> args;
            bool bounded = false;
            for (const profile::Slot &sl : st.slots) {
                if (!sl.hasParam || sl.literal || sl.param != st.param) continue;
                std::set<int64_t> here;
                for (const profile::Bank &b : pr.banks) {
                    if (b.name.size() <= sl.bank.size()) continue;
                    if (b.name.compare(0, sl.bank.size(), sl.bank) != 0) continue;
                    const std::string tail = b.name.substr(sl.bank.size());
                    if (tail.find_first_not_of("0123456789") != std::string::npos) continue;
                    // Un nombre, et un seul, doit reconduire au MEME nom : c'est
                    // `sl.bank + to_string(v)` que `resolveBank` cherchera. Un
                    // `ext01` designerait `ext1`, donc une autre banque que
                    // lui-meme, et un numero interminable n'est pas un numero.
                    if (tail.size() > 9) continue;
                    if (tail.size() > 1 && tail[0] == '0') continue;
                    here.insert(std::stoll(tail));
                }
                if (!bounded) { args = here; bounded = true; continue; }
                std::set<int64_t> both;
                for (int64_t v : args) if (here.count(v)) both.insert(v);
                args.swap(both);
            }
            if (!bounded) continue;
            for (int64_t v : args)
                for (const profile::Slot &sl : st.slots)
                    emit(ax.name, st, true, v, &sl, {st.name + "_" + std::to_string(v)});
        }
    }

    // UN PORT SANS SA VALEUR N'EST PAS UNE OFFRE. Le port et le masque d'un état
    // sont les mêmes par toutes ses fenêtres là où la valeur peut différer, si
    // bien que la règle de retrait n'effaçait que la valeur — et le source se
    // retrouvait avec la moitié d'un couple que le §12.3 emploie d'un bloc,
    // `ld bc, __port_x + __val_x`. Le masque, lui, reste : il appartient à
    // l'AXE, pas à l'état, et le retirer parce qu'un état est ambigu le rendrait
    // indisponible pour les états qui ne le sont pas.
    for (const std::string &n : profileAmbiguous) {
        if (n.compare(0, 6, "__val_") != 0) continue;
        const std::string key = n.substr(6);
        fromProfile.erase("__port_" + key);
        fromProfile.erase("__port2_" + key);
        fromProfile.erase("__romnum_" + key);
    }

    // LA FUSION, ET SON SENS UNIQUE : le profil ne RETIRE jamais ce que le
    // script a offert, et ne le remplace pas non plus.
    //
    // La graphie « par etat » est offerte par les deux chemins, et ils ne
    // repondent pas a la meme question : le script la calcule POUR LA FENETRE
    // qu'il a placee, le profil pour toutes les fenetres de l'etat. Un etat dont
    // deux fenetres donnent deux valeurs ferait donc, par la regle de retrait,
    // DISPARAITRE un symbole qui marchait — le script, lui, savait laquelle.
    //
    // Le script est le plus specifique des deux : il gagne. Et un nom que le
    // script a lui-meme retire reste retire — ce retrait est une decision, pas
    // un manque a combler.
    for (const auto &kv : fromProfile)
        if (!out.count(kv.first) && !ambiguous.count(kv.first)) out[kv.first] = kv.second;
    return out;
}

} // namespace

// --- LE PLACEMENT QUE LE SOURCE PORTE, VERSÉ DANS LA CARTE ------------------
// Rien ici ne place. On construit une CARTE — la même structure qu'un `.ld`
// produit — et tout ce qui suit s'applique sans savoir d'où elle vient : le
// chevauchement, le mou chiffré, l'`ORG` déduit, le découpage. C'est ce qui
// fait de `IN` une seconde syntaxe d'entrée et non un second moteur, et c'est
// aussi ce qui garantit qu'un même défaut se diagnostique d'une seule voix.
std::string statesOf(const profile::Profile &pr) {
    // UN NOM QUE DEUX AXES PORTENT est qualifie de son axe, les autres non. Un
    // profil a plusieurs axes de recouvrement nomme volontiers `on` et `off`
    // dans chacun : les lister nus ferait une liste ou le meme mot revient
    // deux fois sans dire lequel est lequel, et un lecteur ne saurait pas que
    // `rom_upper.on<n>` demande sa qualification pour designer une chose.
    std::map<std::string, int> seen;
    for (const profile::Axis &ax : pr.axes)
        for (const profile::State &st : ax.states) ++seen[st.name];
    std::string list;
    for (const profile::Axis &ax : pr.axes)
        for (const profile::State &st : ax.states) {
            if (!list.empty()) list += ", ";
            if (seen[st.name] > 1) list += ax.name + ".";
            list += st.name;
            if (st.hasParam) list += "<" + st.param + ">";
        }
    return list;
}
std::string windowsOf(const profile::State &st) {
    std::string list;
    for (const profile::Slot &sl : st.slots) {
        if (!list.empty()) list += ", ";
        list += sl.window;
    }
    return list;
}

script::Script withSourcePlacements(const script::Script &sc,
                                    const std::vector<asmb::Object> &objects,
                                    const profile::Profile &pr,
                                    std::vector<asmb::Diagnostic> &errors,
                                    std::vector<asmb::Diagnostic> &warnings) {
    script::Script out = sc;
    // CE QUE LE SCRIPT PLACE DÉJÀ lui appartient : il est plus tardif et plus
    // spécifique que le source, et reprendre un source dont on ne veut pas
    // éditer les sections est un usage réel — le refus l'interdirait.
    //
    // Mais la surcharge ne peut pas être MUETTE, et le piège n'est pas cosmétique :
    // un source qui se place lui-même commute avec la valeur de SA configuration
    // — `__val_ram_ext_w1_1` —, qui est la mauvaise dès que le script l'a posé
    // ailleurs. La faute est alors indétectable à la lecture des deux fichiers
    // pris séparément, et ne se voit qu'à l'exécution.
    std::map<std::string, std::pair<std::string, int>> byScript;   // section -> (fichier, ligne)
    for (const script::ConfigBlock &cb : sc.map)
        for (const script::Placement &p : cb.placements)
            for (const std::string &n : p.sections) byScript[n] = {p.file, p.line};

    // La même identité qu'une clé de commutation : l'axe résolu, l'état, et
    // l'argument. Deux graphies — `ext_w1<1>` et `ram.ext_w1<1>` — nomment une
    // seule configuration, et doivent atterrir dans un seul bloc.
    auto identity = [&](const script::ConfigRef &r) {
        std::string axis;
        if (!findState(pr, r, axis)) axis = r.axis;
        return axis + "." + r.state + (r.hasArg ? "<" + std::to_string(r.arg) + ">" : "");
    };

    // Un nom de section est placé UNE fois, même si plusieurs unités le
    // déclarent — c'est la fusion par nom de C1.0. Deux unités qui le placent
    // AILLEURS l'une que l'autre sont en revanche en désaccord, et le taire
    // ferait dépendre le programme de l'ordre des fichiers.
    std::map<std::string, std::pair<std::string, std::string>> placedBy;
    for (const asmb::Object &obj : objects) {
        for (const asmb::Section &sec : obj.sections) {
            if (sec.place.empty()) continue;
            const std::string want = sec.placeWindow.empty()
                ? sec.place : sec.placeWindow + " OF " + sec.place;

            script::ConfigRef ref;
            std::string err;
            if (!script::parseConfigRef(sec.place, ref, err)) {
                errors.push_back({sec.file, sec.line, "section '" + sec.name + "': " + err});
                continue;
            }
            std::string axisName;
            const profile::State *st = findState(pr, ref, axisName);
            if (!st) {
                errors.push_back({sec.file, sec.line,
                    "section '" + sec.name + "' is placed 'IN " + sec.place +
                    "', which the profile does not declare (it declares: " +
                    statesOf(pr) + ")"});
                continue;
            }
            // LA FENÊTRE. La forme courte ne vaut que si la configuration n'en
            // mappe qu'une : sinon la section irait où ? Le refus nomme les
            // fenêtres et la forme qui tranche, plutôt que d'en choisir une.
            std::string window = sec.placeWindow;
            if (window.empty()) {
                if (st->slots.empty()) {
                    errors.push_back({sec.file, sec.line,
                        "section '" + sec.name + "' is placed 'IN " + sec.place +
                        "', a configuration that maps no window: nothing can be placed in it"});
                    continue;
                }
                if (st->slots.size() > 1) {
                    errors.push_back({sec.file, sec.line,
                        "section '" + sec.name + "' is placed 'IN " + sec.place +
                        "', which maps several windows (" + windowsOf(*st) +
                        "): name the one you mean — 'IN <window> OF " + sec.place + "'"});
                    continue;
                }
                window = st->slots.front().window;
            } else {
                bool mapped = false;
                for (const profile::Slot &sl : st->slots) if (sl.window == window) mapped = true;
                if (!mapped) {
                    errors.push_back({sec.file, sec.line,
                        "section '" + sec.name + "' is placed in window '" + window +
                        "', which the configuration '" + sec.place + "' does not map (it maps: " +
                        (st->slots.empty() ? std::string("none") : windowsOf(*st)) + ")"});
                    continue;
                }
            }

            // DEUX UNITES QUI PLACENT LE MEME NOM se comparent sur l'identité
            // RESOLUE, et non sur le texte : `ext_w1<1>` et `ram.ext_w1<1>`
            // nomment une seule configuration, et le versement, quelques lignes
            // plus bas, les traite déjà comme une seule. Comparer les graphies
            // refuserait un accord parfait pour une différence d'écriture.
            const std::string me = identity(ref) + "/" + window;
            auto seen = placedBy.find(sec.name);
            if (seen != placedBy.end()) {
                if (seen->second.first != me)
                    errors.push_back({sec.file, sec.line,
                        "section '" + sec.name + "' is placed 'IN " + want +
                        "' here and 'IN " + seen->second.second + "' by another unit: a "
                        "section is placed once, and the two would not agree"});
                continue;
            }
            placedBy[sec.name] = {me, want};
            auto over = byScript.find(sec.name);
            if (over != byScript.end()) {
                warnings.push_back({sec.file, sec.line,
                    "section '" + sec.name + "' is placed 'IN " + want +
                    "' here, and the script places it too: the script wins, and this "
                    "'IN' does not apply — a switching value derived from '" + sec.place +
                    "' would be wrong for where the section actually lands"});
                warnings.push_back({over->second.first, over->second.second,
                    "this is the placement of section '" + sec.name + "' that wins"});
                continue;
            }

            // Verser dans le bloc qui existe déjà, quand il existe : deux blocs
            // pour une même configuration et une même fenêtre seraient lus par
            // le contrôle de recouvrement comme deux grilles qui se disputent
            // la banque — un refus dont aucune des deux lignes ne serait fautive.
            script::Placement *slot = nullptr;
            for (script::ConfigBlock &cb : out.map) {
                if (identity(cb.config) != identity(ref)) continue;
                for (script::Placement &p : cb.placements)
                    if (p.window == window && !p.hasRange) slot = &p;
                if (!slot) {
                    script::Placement p;
                    p.window = window;
                    p.file = sec.file;
                    p.line = sec.line;
                    cb.placements.push_back(std::move(p));
                    slot = &cb.placements.back();
                }
                break;
            }
            if (!slot) {
                script::ConfigBlock cb;
                cb.config = ref;
                cb.file = sec.file;
                cb.line = sec.line;
                script::Placement p;
                p.window = window;
                p.file = sec.file;
                p.line = sec.line;
                cb.placements.push_back(std::move(p));
                out.map.push_back(std::move(cb));
                slot = &out.map.back().placements.back();
            }
            slot->sections.push_back(sec.name);
        }
    }
    return out;
}

Image build(const std::vector<asmb::Object> &objects,
            const script::Script &sc, const profile::Profile &pr) {
    Linker lk;
    // Un script qui PLACE sans profil ne peut rien calculer : la fenêtre, la
    // banque et le numéro de rangement viennent tous du profil. Le dire ici est
    // ce qui empêche un script d'être accepté et ignoré.
    if (!sc.map.empty() && pr.windows.empty()) {
        lk.out.ok = false;
        lk.out.errors.push_back({sc.map.front().file, sc.map.front().line,
            "a script that places sections needs a profile: the window gives the "
            "address, the configuration gives the bank, and the profile gives both "
            "— pass --target or -P"});
        return lk.out;
    }
    // Les objets se posent dans l'ordre où on les a donnés. Leurs tables de
    // sites se concatènent, et `prov` se décale d'autant : c'est ce qui laisse
    // un recouvrement nommer la bonne ligne du bon fichier.
    // Les noms d'abord : le placement lui-meme diagnostique — deux unites qui
    // declarent le meme nom de section sans en dire la meme chose — et un
    // diagnostic qui ne peut pas nommer son unite ne sert a rien.
    for (const asmb::Object &obj : objects) lk.objectNames_.push_back(obj.name);

    // LE PLACEMENT QUE LE SOURCE PORTE rejoint la carte du script, AVANT que
    // quoi que ce soit ne place. Un seul moteur en aval, donc un seul jeu de
    // diagnostics — et le contrôle qui le prouve est que rien, plus bas, n'a eu
    // besoin de savoir d'où vient une section.
    bool sourcePlaces = false;
    for (const asmb::Object &obj : objects)
        for (const asmb::Section &sec : obj.sections)
            if (!sec.place.empty()) sourcePlaces = true;
    if (sourcePlaces && pr.windows.empty()) {
        const asmb::Section *first = nullptr;
        for (const asmb::Object &obj : objects)
            for (const asmb::Section &sec : obj.sections)
                if (!first && !sec.place.empty()) first = &sec;
        lk.out.ok = false;
        lk.out.errors.push_back({first->file, first->line,
            "section '" + first->name + "' places itself 'IN " + first->place +
            "', which needs a profile: the window gives the address, the configuration "
            "gives the bank, and the profile gives both — pass --target or -P"});
        return lk.out;
    }
    script::Script placement = sc;
    if (sourcePlaces) {
        placement = withSourcePlacements(sc, objects, pr, lk.out.errors, lk.out.warnings);
        if (!lk.out.errors.empty()) { lk.out.ok = false; return lk.out; }
    }

    // Trois temps, et l'ordre est force. D'abord : ou vont les sections que
    // personne n'a placees.
    lk.bases_ = lk.placeRelocSections(objects, placement, sc, pr);

    // Ensuite : ce que les objets EXPORTENT. Il faut que TOUTES les bases soient
    // decidees avant, sans quoi un `PUBLIC` d'une section relocalisable n'aurait
    // pas encore d'adresse a offrir.
    std::map<std::string, size_t> exportedBy;
    for (size_t oi = 0; oi < objects.size(); ++oi) {
        lk.relocBase_ = lk.bases_[oi];
        lk.relocStore_ = lk.stores_[oi];
        for (const asmb::Symbol &s : objects[oi].symbolTable) {
            if (!s.isPublic) continue;
            // Deux objets qui exportent le même nom : refusé, en nommant LES
            // DEUX provenances. En choisir un silencieusement ferait dépendre le
            // programme de l'ordre des fichiers sur la ligne de commande.
            auto prev = exportedBy.find(s.name);
            if (prev != exportedBy.end()) {
                lk.out.errors.push_back({obj_name(objects[oi]), s.line,
                    "symbol '" + s.name + "' is exported twice: by " +
                    lk.objectLabel((int)prev->second) + " and by " + lk.objectLabel((int)oi)});
                continue;
            }
            exportedBy[s.name] = oi;
            lk.exported_[s.name] = lk.symbolAddress(objects[oi], s);
        }
    }

    // Enfin : ecrire dans les octets ce que ces decisions rendent calculable, et
    // poser.
    for (size_t oi = 0; oi < objects.size(); ++oi) {
        const asmb::Object &obj = objects[oi];
        const uint16_t base = (uint16_t)lk.sites_.size();
        for (const asmb::Site &s : obj.sites) lk.sites_.push_back(s);
        lk.relocBase_ = lk.bases_[oi];
        lk.relocStore_ = lk.stores_[oi];
        lk.curObject_ = (int)oi;
        asmb::Object placed = obj;
        lk.resolve(placed.fragments, obj);
        lk.place(placed, base);
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
    int entryFrom = -1;
    for (size_t oi = 0; oi < objects.size(); ++oi) {
        const asmb::Object &obj = objects[oi];
        if (!obj.entry.has) continue;
        lk.relocBase_ = lk.bases_[oi];
        lk.relocStore_ = lk.stores_[oi];
        // Le point d'entrée passe par `symbolAddress`, et non par la table crue
        // `obj.symbols` : dans une section RELOCALISABLE, celle-ci ne porte qu'un
        // OFFSET, et prendre cet offset pour une adresse faisait démarrer le PC à
        // l'offset zéro de la section.
        //
        // Le défaut existait depuis l'étage B, où il ne se voyait pas : les
        // sections relocalisables s'y posaient après le dernier octet absolu,
        // donc à la base zéro pour une source qui n'en a aucun. Le placement
        // calculé le rend certain.
        int run = (int)obj.entry.value;
        if (!obj.entry.name.empty()) {
            bool found = false;
            for (const asmb::Symbol &s : obj.symbolTable)
                if (s.name == obj.entry.name) { run = (int)lk.symbolAddress(obj, s); found = true; break; }
            if (!found) {
                auto it = obj.symbols.find(obj.entry.name);
                if (it != obj.symbols.end()) run = (int)it->second;
            }
        }
        // Deux `run` : refusé. En choisir un ferait dépendre le point d'entrée
        // de l'ordre des fichiers, ce qu'aucun auteur n'a écrit.
        if (entryFrom >= 0) {
            lk.out.errors.push_back({obj.entry.file, obj.entry.line,
                "two objects declare an entry point: " + lk.objectLabel(entryFrom) +
                " and " + lk.objectLabel((int)oi)});
            continue;
        }
        entryFrom = (int)oi;
        lk.out.runAddress = (uint16_t)run;
        lk.warnRunDisplaced(obj, run);
    }

    // La table des symboles, avec des adresses DÉFINITIVES. C'est ici qu'elle se
    // fabrique parce que c'est ici que les fragments sont placés : un label vaut
    // l'adresse de son fragment plus son offset, et l'assembleur ne connaît que
    // le second terme. Le format, lui, ne bouge pas (amendement à l'ADR 0019).
    for (size_t oi = 0; oi < objects.size(); ++oi) {
        const asmb::Object &obj = objects[oi];
        lk.relocBase_ = lk.bases_[oi];
        lk.relocStore_ = lk.stores_[oi];
        for (const asmb::Symbol &s : obj.symbolTable) {
            Symbol out;
            out.name = s.name;
            out.isConst = s.isConst;
            out.value = s.value;
            out.section = s.section;
            out.file = s.file;
            out.line = s.line;
            // Une constante n'habite nulle part : ni banque, ni rangement.
            if (!s.isConst && s.frag >= 0 && (size_t)s.frag < obj.fragments.size()) {
                const asmb::Fragment &f = obj.fragments[(size_t)s.frag];
                const int fragAddr = f.addr + lk.baseOf(f.relocSection);
                out.store = (fragAddr + s.offset) & 0xFFFF;
                const int forced = lk.storeOf(f.relocSection);
                out.bank = forced >= 0 ? forced
                                       : (f.bank < 0 ? ((out.store >> 14) & 3) : f.bank);
                // Dans une section relocalisable, la VALEUR aussi n'etait
                // connue que d'ici : c'est tout l'objet de l'amendement a
                // l'ADR 0019.
                if (f.relocSection >= 0) out.value = lk.symbolAddress(obj, s);
            }
            lk.out.symbolTable.push_back(std::move(out));
        }
    }

    lk.out.ok = lk.out.errors.empty();
    return lk.out;
}

// Le calcul du §12.3, expose : le CLI l'appelle AVANT d'assembler, et le linker
// s'en sert pour ses `EXTERN`. Une seule fonction, donc une seule valeur.
std::map<std::string, int64_t> switchSymbols(const script::Script &script,
                                             const profile::Profile &profile) {
    return compute(script, profile);
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
