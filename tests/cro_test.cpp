// cro_test.cpp - Tests de l'export conteneur de ROMs CRO (ADR 0033)
#include "cro.h"

#include "link.h"
#include "profile.h"
#include "script.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;
static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass; else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

// Un objet d'une section relocalisable `rom`, portant `bytes`.
static asmb::Object romObject(const char *unit, std::vector<uint8_t> bytes) {
    asmb::Object o;
    o.name = unit;
    o.sites.push_back({unit, 1});
    asmb::Fragment f;
    f.placed = false;
    f.relocSection = 0;
    f.section = "rom";
    f.bytes = std::move(bytes);
    f.prov.assign(f.bytes.size(), 1);
    o.fragments.push_back(f);
    asmb::Section sec;
    sec.name = "rom"; sec.id = 0; sec.relocatable = true; sec.kind = "RO";
    sec.size = (int64_t)f.bytes.size(); sec.file = unit; sec.line = 1;
    o.sections.push_back(sec);
    return o;
}

// L'image d'un objet placé par `config` (« rom_upper.on<7> { w3 », …).
static link::Image placed(const profile::Profile &pr, const std::string &config,
                          std::vector<uint8_t> bytes) {
    const std::string map = "MEMORY_MAP { CONFIG " + config + " { SECTION rom } } }";
    return link::build({romObject("r.fo", std::move(bytes))}, script::parse(map, "r.ld"), pr);
}

// --- l'ecriture attendue, octet par octet (formes des fichiers CROMANAGER) --
static void put(std::vector<uint8_t> &v, const char *fourcc) {
    v.insert(v.end(), fourcc, fourcc + 4);
}
static void put32(std::vector<uint8_t> &v, uint32_t x) {
    for (int i = 0; i < 4; ++i) v.push_back((uint8_t)(x >> (8 * i)));
}
static void putU32Chunk(std::vector<uint8_t> &v, const char *id, uint32_t x) {
    put(v, id); put32(v, 4); put32(v, x);
}

// Une ROM « a la main » : 16 Ko, `b0` en tete, zero ailleurs.
static cro::Rom handRom(const char *id, uint32_t type, uint32_t slot, uint32_t phys, uint8_t b0,
                        cro::Family fam = cro::Family::Classic) {
    cro::Rom r;
    r.family = fam; r.id = id; r.type = type; r.slot = slot; r.physical = phys;
    r.data.assign(0x4000, 0);
    r.data[0] = b0;
    return r;
}

// --- la relecture, pour les tests qui ne regardent qu'un champ ------------
// Le n-ieme chunk `id` du fichier (0 = le premier), -1 s'il manque.
static long nth(const std::vector<uint8_t> &b, const char *id, int n) {
    for (size_t i = 0; i + 4 <= b.size(); ++i)
        if (std::memcmp(b.data() + i, id, 4) == 0 && n-- == 0) return (long)i;
    return -1;
}
static uint32_t u32At(const std::vector<uint8_t> &b, long off) {
    return (uint32_t)b[off] | ((uint32_t)b[off + 1] << 8) | ((uint32_t)b[off + 2] << 16) |
           ((uint32_t)b[off + 3] << 24);
}
static std::string text(const std::vector<uint8_t> &b, long off) {
    return std::string((const char *)b.data() + off + 8, u32At(b, off + 4));
}

int main() {
    printf("Tests export CRO\n");

    // --- ROM haute classique : HIGH, slot = physique = n ---------------------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        link::Image img = placed(pr, "rom_upper.on<7> { w3", {0xC9, 0x01});
        ok("rom_hi<7> se lie", img.ok);

        cro::Rom rom;
        std::string err;
        ok("rom_hi<7> s'extrait", cro::extractOne(img, pr, 7, rom, err));
        ok("pas d'erreur", err.empty());
        ok("famille classique", rom.family == cro::Family::Classic);
        ok("RTYP HIGH", rom.type == cro::kHigh);
        ok("RLOG 7", rom.slot == 7);
        ok("RPHY 7", rom.physical == 7);
        ok("RID hi07", rom.id == "hi07");
        ok("16 Ko", rom.data.size() == 0x4000);
        ok("octets recopies en tete de ROM",
           rom.data.size() == 0x4000 && rom.data[0] == 0xC9 && rom.data[1] == 0x01 &&
               rom.data[2] == 0x00 && rom.data.back() == 0x00);
    }

    // --- ROM basse classique : LOW, slot 0, physique 0, sans numero --------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        link::Image img = placed(pr, "rom_lower.on { w0", {0xF3});
        ok("rom_lo se lie", img.ok);

        cro::Rom rom;
        std::string err;
        ok("rom_lo s'extrait sans --cro-rom", cro::extractOne(img, pr, -1, rom, err));
        ok("famille classique (basse)", rom.family == cro::Family::Classic);
        ok("RTYP LOW", rom.type == cro::kLow);
        ok("RLOG 0 (basse)", rom.slot == 0);
        ok("RPHY 0 (basse)", rom.physical == 0);
        ok("RID lo", rom.id == "lo");
        ok("octet de la ROM basse", rom.data.size() == 0x4000 && rom.data[0] == 0xF3);
    }

    // --- ROM de cartouche : physique n, slot 7 pour la 3, type au seuil 8 --
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");

        link::Image img3 = placed(pr, "cart_rom.w0<3> { w0", {0x33});
        ok("crom3 se lie", img3.ok);
        cro::Rom rom3;
        std::string err;
        ok("crom3 s'extrait", cro::extractOne(img3, pr, 3, rom3, err));
        ok("famille cartouche", rom3.family == cro::Family::Cartridge);
        ok("crom3 : RTYP BANKABLE (< 8)", rom3.type == cro::kBankable);
        ok("crom3 : RLOG 7 (transposition du Plus)", rom3.slot == 7);
        ok("crom3 : RPHY 3", rom3.physical == 3);
        ok("crom3 : RID cb03", rom3.id == "cb03");
        ok("crom3 : octet", rom3.data.size() == 0x4000 && rom3.data[0] == 0x33);

        link::Image img0 = placed(pr, "cart_rom.w0<0> { w0", {0x00});
        cro::Rom rom0;
        ok("crom0 s'extrait", cro::extractOne(img0, pr, 0, rom0, err));
        ok("crom0 : RLOG 1", rom0.slot == 1);

        link::Image img19 = placed(pr, "cart_rom_hi.on<19> { w3", {0x99});
        ok("crom19 se lie", img19.ok);
        cro::Rom rom19;
        ok("crom19 s'extrait", cro::extractOne(img19, pr, 19, rom19, err));
        ok("crom19 : RTYP HIGH (>= 8)", rom19.type == cro::kHigh);
        ok("crom19 : RLOG 1", rom19.slot == 1);
        ok("crom19 : RPHY 19", rom19.physical == 19);
        ok("crom19 : RID cb19 (decimal)", rom19.id == "cb19");
        ok("crom19 : octet a sa place dans la banque",
           rom19.data.size() == 0x4000 && rom19.data[0] == 0x99);
    }

    // --- refus : ce qui ne designe pas UNE ROM, ou un numero qui ne lui va pas
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        const profile::Profile plus = profile::parse(profile::builtin("cpcplus"), "cpcplus");
        cro::Rom rom;
        std::string err;

        link::Image ram = placed(pr, "ram.linear { w1", {0x01});
        ok("une image en RAM se lie", ram.ok);
        ok("RAM seule : refus", !cro::extractOne(ram, pr, 7, rom, err));
        ok("RAM seule : l'erreur le dit", err.find("aucune banque de ROM") != std::string::npos);

        // Deux ROMs dans une meme image : laquelle livrer ? On ne choisit pas.
        asmb::Object two = romObject("two.fo", {0xAA});
        {
            asmb::Fragment f = two.fragments[0];
            f.relocSection = 1; f.section = "rom2"; f.bytes = {0xBB};
            two.fragments.push_back(f);
            asmb::Section s = two.sections[0];
            s.name = "rom2"; s.id = 1;
            two.sections.push_back(s);
        }
        link::Image both = link::build({two}, script::parse(
            "MEMORY_MAP { CONFIG rom_lower.on { w0 { SECTION rom } }"
            "             CONFIG rom_upper.on<7> { w3 { SECTION rom2 } } }", "two.ld"), pr);
        ok("ROM basse + haute se lient ensemble", both.ok);
        err.clear();
        ok("deux banques de ROM : refus", !cro::extractOne(both, pr, 7, rom, err));
        ok("deux banques : l'erreur nomme les deux",
           err.find("rom_lo") != std::string::npos && err.find("rom_hi") != std::string::npos);

        link::Image hi = placed(pr, "rom_upper.on<7> { w3", {0x01});
        err.clear();
        ok("rom_hi sans numero : refus", !cro::extractOne(hi, pr, -1, rom, err));
        ok("rom_hi sans numero : l'erreur nomme --cro-rom", err.find("--cro-rom") != std::string::npos);
        err.clear();
        ok("rom_hi 256 : refus", !cro::extractOne(hi, pr, 256, rom, err));
        ok("rom_hi 256 : l'erreur donne la borne", err.find("255") != std::string::npos);
        ok("rom_hi 255 : accepte", cro::extractOne(hi, pr, 255, rom, err));

        link::Image lo = placed(pr, "rom_lower.on { w0", {0x01});
        err.clear();
        ok("rom_lo avec numero : refus", !cro::extractOne(lo, pr, 0, rom, err));
        ok("rom_lo avec numero : l'erreur nomme --cro-rom", err.find("--cro-rom") != std::string::npos);

        link::Image cart = placed(plus, "cart_rom.w0<0> { w0", {0x01});
        err.clear();
        ok("crom sans numero : refus", !cro::extractOne(cart, plus, -1, rom, err));
        err.clear();
        ok("crom 32 : refus", !cro::extractOne(cart, plus, 32, rom, err));
        ok("crom 32 : l'erreur donne la borne", err.find("31") != std::string::npos);
    }

    // --- merge dans un conteneur vide : le fichier complet, a l'octet ------
    {
        std::vector<uint8_t> want;
        put(want, "RIFF"); put32(want, 0x4070); put(want, "CRO ");
        put(want, "GRRO"); put32(want, 0x4064);
        putU32Chunk(want, "GNUM", 0);
        put(want, "GLBL"); put32(want, 4); put(want, "demo");
        putU32Chunk(want, "GMSK", 0xFFFFFFFF);
        put(want, "ROM "); put32(want, 0x4038);
        put(want, "RID "); put32(want, 4); put(want, "hi07");
        putU32Chunk(want, "RTYP", 1);
        putU32Chunk(want, "RLOG", 7);
        putU32Chunk(want, "RPHY", 7);
        put(want, "RDT "); put32(want, 0x4000);
        want.push_back(0xC9); want.insert(want.end(), 0x3FFF, 0);

        std::string err;
        std::vector<uint8_t> out =
            cro::merge({}, handRom("hi07", cro::kHigh, 7, 7, 0xC9), cro::GroupOptions{}, "demo", err);
        ok("merge vide : pas d'erreur", err.empty());
        ok("merge vide : taille 16504", out.size() == 16504);
        ok("merge vide : fichier attendu a l'octet", out == want);
    }

    // --- GLBL de longueur impaire : un octet de padding, taille non comptee -
    {
        std::string err;
        std::vector<uint8_t> out =
            cro::merge({}, handRom("hi07", cro::kHigh, 7, 7, 0), cro::GroupOptions{}, "abc", err);
        // GLBL a l'offset 12 + 8 + 12 = 32 : id, taille 3, « abc », pad.
        ok("GLBL impair : taille 3",
           out.size() > 44 && std::string((char *)out.data() + 32, 4) == "GLBL" &&
               out[36] == 3 && out[37] == 0 && out[38] == 0 && out[39] == 0);
        ok("GLBL impair : un octet nul apres le texte",
           out.size() > 44 && std::string((char *)out.data() + 40, 3) == "abc" && out[43] == 0);
        ok("GLBL impair : GMSK suit le padding",
           out.size() > 48 && std::string((char *)out.data() + 44, 4) == "GMSK");
        ok("GLBL impair : GRRO compte le padding", out[16] == 0x64 && out[17] == 0x40);
    }

    // --- merge dans un fichier existant : ajout a la suite, remplacement sur place
    {
        std::string err;
        const cro::GroupOptions g{};
        std::vector<uint8_t> one = cro::merge({}, handRom("hi07", cro::kHigh, 7, 7, 0x07), g, "demo", err);
        std::vector<uint8_t> two = cro::merge(one, handRom("hi15", cro::kHigh, 15, 15, 0x15), g, "demo", err);
        ok("ajout : pas d'erreur", err.empty());
        ok("ajout : une ROM de plus (16448 octets)", two.size() == 16504 + 16448);
        ok("ajout : taille RIFF", two.size() >= 8 && two[4] == 0xB0 && two[5] == 0x80 && two[6] == 0);
        ok("ajout : taille GRRO", two.size() >= 20 && two[16] == 0xA4 && two[17] == 0x80 && two[18] == 0);
        ok("ajout : la premiere ROM reste en tete", two.size() > 16568 && two[120] == 0x07);
        ok("ajout : la seconde suit",
           two.size() > 16568 && std::string((char *)two.data() + 16504, 4) == "ROM " &&
               std::string((char *)two.data() + 16520, 4) == "hi15" && two[16568] == 0x15);

        std::vector<uint8_t> three =
            cro::merge(two, handRom("hi07", cro::kHigh, 7, 7, 0x55), g, "demo", err);
        ok("remplacement : pas d'erreur", err.empty());
        ok("remplacement : meme taille", three.size() == two.size());
        ok("remplacement : a sa place", three.size() > 16568 && three[120] == 0x55);
        ok("remplacement : l'autre ROM intacte", three.size() > 16568 && three[16568] == 0x15);
    }

    // --- un fichier a la CROMANAGER, avec des chunks que fantams ignore -----
    // RID et GLBL impairs (padding), GMSK 64 K, et trois chunks inconnus : au
    // niveau du fichier, d'un groupe et d'une ROM.
    {
        auto romBody = [](const char *rid, uint32_t type, uint32_t slot, uint32_t phys,
                          uint8_t b0, bool extra) {
            std::vector<uint8_t> r;
            const uint32_t n = (uint32_t)strlen(rid);
            put(r, "RID "); put32(r, n); r.insert(r.end(), rid, rid + n);
            if (n % 2) r.push_back(0);
            putU32Chunk(r, "RTYP", type);
            putU32Chunk(r, "RLOG", slot);
            putU32Chunk(r, "RPHY", phys);
            put(r, "RDT "); put32(r, 0x4000);
            r.push_back(b0); r.insert(r.end(), 0x3FFF, 0);
            if (extra) { put(r, "RCRC"); put32(r, 3); r.push_back(1); r.push_back(2); r.push_back(3); r.push_back(0); }
            return r;
        };
        std::vector<uint8_t> grro;
        putU32Chunk(grro, "GNUM", 0);
        put(grro, "GLBL"); put32(grro, 13); put(grro, "Cart"); put(grro, "ouch"); put(grro, "e.cp");
        grro.push_back('r'); grro.push_back(0);
        putU32Chunk(grro, "GMSK", 0xFFFF);
        put(grro, "XTRA"); put32(grro, 2); grro.push_back(0xAB); grro.push_back(0xCD);
        for (auto body : {romBody("BASIC.ROM", cro::kBankable, 1, 0, 0xB0, false),
                          romBody("cb03", cro::kBankable, 7, 3, 0x03, true)}) {
            put(grro, "ROM "); put32(grro, (uint32_t)body.size());
            grro.insert(grro.end(), body.begin(), body.end());
        }
        std::vector<uint8_t> file;
        put(file, "RIFF"); put32(file, 0); put(file, "CRO ");
        put(file, "ZZZZ"); put32(file, 1); file.push_back(0x5A); file.push_back(0);
        put(file, "GRRO"); put32(file, (uint32_t)grro.size());
        file.insert(file.end(), grro.begin(), grro.end());
        const uint32_t riffSize = (uint32_t)file.size() - 8;
        for (int i = 0; i < 4; ++i) file[4 + i] = (uint8_t)(riffSize >> (8 * i));

        std::string err;
        const cro::Rom same = handRom("cb03", cro::kBankable, 7, 3, 0x03, cro::Family::Cartridge);
        std::vector<uint8_t> out = cro::merge(file, same, cro::GroupOptions{}, "ignore", err);
        ok("identite : pas d'erreur", err.empty());
        ok("identite : refusionner la meme ROM rend le fichier a l'octet", out == file);

        const cro::Rom changed = handRom("cb03", cro::kBankable, 7, 3, 0x77, cro::Family::Cartridge);
        out = cro::merge(file, changed, cro::GroupOptions{}, "ignore", err);
        ok("inconnus : meme taille apres remplacement", out.size() == file.size());
        std::vector<size_t> diff;
        for (size_t i = 0; i < out.size() && i < file.size(); ++i)
            if (out[i] != file[i]) diff.push_back(i);
        size_t at = 0;
        for (size_t i = 0; i + 4 <= file.size(); ++i)
            if (std::string((char *)file.data() + i, 4) == "cb03") at = i;
        // cb03 est suivi de RTYP, RLOG, RPHY (36 octets) puis de l'en-tete RDT.
        ok("inconnus : seul le premier octet de RDT a change",
           diff.size() == 1 && out[at + 4 + 36 + 8] == 0x77);
        ok("inconnus : ZZZZ, XTRA et RCRC conserves", std::string((char *)out.data() + 12, 4) == "ZZZZ" &&
               std::search(out.begin(), out.end(), "XTRA", "XTRA" + 4) != out.end() &&
               std::search(out.begin(), out.end(), "RCRC", "RCRC" + 4) != out.end());
    }

    // --- groupes : un second groupe, et ce que les drapeaux changent -------
    {
        std::string err;
        std::vector<uint8_t> f0 = cro::merge({}, handRom("hi07", cro::kHigh, 7, 7, 0), {}, "base", err);

        cro::GroupOptions g1;
        g1.group = 1;
        g1.hasLabel = true; g1.label = "Utilitaires";
        g1.hasMask = true; g1.mask = 0x1FFFF;
        std::vector<uint8_t> f1 = cro::merge(f0, handRom("hi07", cro::kHigh, 7, 7, 0x11), g1, "base", err);
        ok("groupe 1 : pas d'erreur", err.empty());
        ok("groupe 1 : deux GRRO", nth(f1, "GRRO", 1) > nth(f1, "GRRO", 0) && nth(f1, "GRRO", 0) == 12);
        ok("groupe 1 : GNUM 1", u32At(f1, nth(f1, "GNUM", 1) + 8) == 1);
        ok("groupe 1 : libelle declare", text(f1, nth(f1, "GLBL", 1)) == "Utilitaires");
        ok("groupe 1 : masque declare", u32At(f1, nth(f1, "GMSK", 1) + 8) == 0x1FFFF);
        ok("groupe 1 : le groupe 0 garde sa ROM 7", f1[nth(f1, "RDT ", 0) + 8] == 0x00);
        ok("groupe 1 : sa propre ROM 7", f1[nth(f1, "RDT ", 1) + 8] == 0x11);

        // Sans drapeau, un groupe existant garde libelle et masque.
        cro::GroupOptions again;
        again.group = 1;
        std::vector<uint8_t> f2 = cro::merge(f1, handRom("hi08", cro::kHigh, 8, 8, 0), again, "autre", err);
        ok("sans drapeau : libelle conserve", text(f2, nth(f2, "GLBL", 1)) == "Utilitaires");
        ok("sans drapeau : masque conserve", u32At(f2, nth(f2, "GMSK", 1) + 8) == 0x1FFFF);

        // Avec drapeau, il les remplace, a leur place.
        cro::GroupOptions over;
        over.group = 0;
        over.hasLabel = true; over.label = "Systeme";
        over.hasMask = true; over.mask = 0xFFFF;
        std::vector<uint8_t> f3 = cro::merge(f2, handRom("hi07", cro::kHigh, 7, 7, 0), over, "x", err);
        ok("drapeau : libelle remplace", text(f3, nth(f3, "GLBL", 0)) == "Systeme");
        ok("drapeau : masque remplace", u32At(f3, nth(f3, "GMSK", 0) + 8) == 0xFFFF);
        ok("drapeau : GLBL reste entre GNUM et GMSK",
           nth(f3, "GNUM", 0) < nth(f3, "GLBL", 0) && nth(f3, "GLBL", 0) < nth(f3, "GMSK", 0));
        ok("drapeau : l'autre groupe intact", text(f3, nth(f3, "GLBL", 1)) == "Utilitaires");
    }

    // --- groupes relus sans GNUM, GLBL ni GMSK : rang, puis completes -------
    {
        auto bareGroup = [](uint8_t b0) {
            std::vector<uint8_t> rom;
            put(rom, "RID "); put32(rom, 4); put(rom, "hi07");
            putU32Chunk(rom, "RTYP", cro::kHigh);
            putU32Chunk(rom, "RLOG", 7);
            putU32Chunk(rom, "RPHY", 7);
            put(rom, "RDT "); put32(rom, 0x4000); rom.push_back(b0); rom.insert(rom.end(), 0x3FFF, 0);
            std::vector<uint8_t> g;
            put(g, "GRRO"); put32(g, 8 + (uint32_t)rom.size());
            put(g, "ROM "); put32(g, (uint32_t)rom.size());
            g.insert(g.end(), rom.begin(), rom.end());
            return g;
        };
        std::vector<uint8_t> file;
        put(file, "RIFF"); put32(file, 0); put(file, "CRO ");
        for (uint8_t b : {0xA0, 0xA1}) {
            std::vector<uint8_t> g = bareGroup(b);
            file.insert(file.end(), g.begin(), g.end());
        }
        const uint32_t riffSize = (uint32_t)file.size() - 8;
        for (int i = 0; i < 4; ++i) file[4 + i] = (uint8_t)(riffSize >> (8 * i));

        cro::GroupOptions second;
        second.group = 1;
        std::string err;
        std::vector<uint8_t> out =
            cro::merge(file, handRom("hi07", cro::kHigh, 7, 7, 0xB1), second, "rang1", err);
        ok("groupe nu : pas d'erreur", err.empty());
        ok("groupe nu : le rang 1 est le second GRRO",
           out[nth(out, "RDT ", 0) + 8] == 0xA0 && out[nth(out, "RDT ", 1) + 8] == 0xB1);
        ok("groupe nu : pas de groupe cree", nth(out, "GRRO", 2) < 0);
        const long g1 = nth(out, "GRRO", 1);
        ok("groupe nu : le groupe touche est complete en tete, dans l'ordre",
           g1 > 0 && nth(out, "GNUM", 0) == g1 + 8 && nth(out, "GLBL", 0) == g1 + 20 &&
               nth(out, "GMSK", 0) > nth(out, "GLBL", 0) && nth(out, "GMSK", 0) < nth(out, "ROM ", 1));
        ok("groupe nu : GNUM = son rang", nth(out, "GNUM", 0) > 0 && u32At(out, nth(out, "GNUM", 0) + 8) == 1);
        ok("groupe nu : GLBL par defaut", nth(out, "GLBL", 0) > 0 && text(out, nth(out, "GLBL", 0)) == "rang1");
        ok("groupe nu : GMSK sans masque",
           nth(out, "GMSK", 0) > 0 && u32At(out, nth(out, "GMSK", 0) + 8) == 0xFFFFFFFF);
        ok("groupe nu : l'autre groupe reste nu", nth(out, "GNUM", 1) < 0);
    }

    // --- refus de merge : familles melees, fichier qui n'est pas un CRO -----
    {
        std::string err;
        const cro::Rom cart7 = handRom("cb07", cro::kBankable, 1, 7, 0xC7, cro::Family::Cartridge);
        std::vector<uint8_t> classic = cro::merge({}, handRom("hi07", cro::kHigh, 7, 7, 0x07), {}, "c", err);

        std::vector<uint8_t> out = cro::merge(classic, cart7, {}, "c", err);
        ok("melange : refus", out.empty());
        ok("melange : l'erreur nomme le groupe et les deux familles",
           err.find("groupe 0") != std::string::npos && err.find("cartouche") != std::string::npos &&
               err.find("classique") != std::string::npos);

        cro::GroupOptions other;
        other.group = 1;
        out = cro::merge(classic, cart7, other, "p", err);
        ok("familles dans deux groupes : accepte", !out.empty() && err.empty());

        // Une ROM venue d'un autre outil (RID quelconque) ne fixe pas la famille :
        // elle se remplace sur son numero physique.
        std::vector<uint8_t> foreign =
            cro::merge({}, handRom("BASIC.ROM", cro::kBankable, 1, 7, 0x42), {}, "f", err);
        out = cro::merge(foreign, cart7, {}, "f", err);
        ok("RID inconnu : pas de refus", !out.empty() && err.empty());
        ok("RID inconnu : remplacee sur son numero physique",
           nth(out, "ROM ", 1) < 0 && out[nth(out, "RDT ", 0) + 8] == 0xC7);

        out = cro::merge(std::vector<uint8_t>{'R', 'I', 'F', 'F', 4, 0, 0, 0, 'A', 'M', 'S', '!'},
                         cart7, {}, "x", err);
        ok("un CPR n'est pas un CRO : refus", out.empty() && err.find("CRO") != std::string::npos);

        std::vector<uint8_t> cut = classic;
        cut.resize(cut.size() - 100);
        out = cro::merge(cut, cart7, other, "x", err);
        ok("fichier tronque : refus", out.empty() && err.find("tronque") != std::string::npos);
    }

    // --- ROM basse et ROM haute 0 : meme numero physique, deux ROMs --------
    // La ROM basse n'est pas une ROM haute : la cle d'un groupe est le numero
    // physique ET le fait d'etre la ROM basse (ADR 0033).
    {
        std::string err;
        std::vector<uint8_t> f = cro::merge({}, handRom("lo", cro::kLow, 0, 0, 0x10), {}, "sys", err);
        f = cro::merge(f, handRom("hi00", cro::kHigh, 0, 0, 0x20), {}, "sys", err);
        ok("lo + hi00 : pas d'erreur", err.empty());
        ok("lo + hi00 : deux ROMs", nth(f, "ROM ", 1) > 0 && nth(f, "ROM ", 2) < 0);
        ok("lo + hi00 : lo garde ses octets", f[nth(f, "RDT ", 0) + 8] == 0x10);
        ok("lo + hi00 : hi00 a les siens", f[nth(f, "RDT ", 1) + 8] == 0x20);

        f = cro::merge(f, handRom("hi00", cro::kHigh, 0, 0, 0x21), {}, "sys", err);
        ok("hi00 refusionnee : remplace hi00, pas lo",
           nth(f, "ROM ", 2) < 0 && f[nth(f, "RDT ", 0) + 8] == 0x10 && f[nth(f, "RDT ", 1) + 8] == 0x21);
        f = cro::merge(f, handRom("lo", cro::kLow, 0, 0, 0x11), {}, "sys", err);
        ok("lo refusionnee : remplace lo, pas hi00",
           nth(f, "ROM ", 2) < 0 && f[nth(f, "RDT ", 0) + 8] == 0x11 && f[nth(f, "RDT ", 1) + 8] == 0x21);
    }

    // --- une ROM haute a trois chiffres reste classique --------------------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        link::Image img = placed(pr, "rom_upper.on<100> { w3", {0x01});
        cro::Rom hi100;
        std::string err;
        ok("rom_hi<100> s'extrait", cro::extractOne(img, pr, 100, hi100, err));
        ok("rom_hi<100> : RID hi100", hi100.id == "hi100");

        std::vector<uint8_t> f = cro::merge({}, hi100, {}, "sys", err);
        std::vector<uint8_t> out =
            cro::merge(f, handRom("cb07", cro::kBankable, 1, 7, 0, cro::Family::Cartridge), {}, "sys", err);
        ok("hi100 puis cartouche dans le meme groupe : refus",
           out.empty() && err.find("classique") != std::string::npos);
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
