// cpr_test.cpp - Tests de l'export cartouche CPR
#include "cpr.h"

#include "link.h"
#include "profile.h"
#include "script.h"

#include <cstdio>
#include <map>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;
static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass; else { ++g_fail; printf("  \033[31mFAIL\033[0m %s\n", desc); }
}

int main() {
    printf("Tests export CPR\n");

    // --- profil sans axe cart_rom : refus nommant l'axe manquant -----------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        std::map<int, link::Image> perBank;
        std::string err;
        std::vector<uint8_t> out = cpr::build(perBank, pr, err);

        ok("aucun octet quand l'axe manque", out.empty());
        ok("l'erreur nomme cart_rom", err.find("cart_rom") != std::string::npos);
    }

    // --- une seule banque physique écrite : un chunk cb00 ------------------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");
        const script::Script scr =
            script::parse("MEMORY_MAP { CONFIG cart_rom.w0<0> { w0 { SECTION cart } } }", "b0.ld");

        asmb::Object o;
        o.name = "b0.fo";
        o.sites.push_back({"b0.fo", 1});
        asmb::Fragment f;
        f.placed = false;
        f.relocSection = 0;
        f.section = "cart";
        f.bytes = {0xAA, 0xBB, 0xCC};
        f.prov.assign(f.bytes.size(), 1);
        o.fragments.push_back(f);
        asmb::Section sec;
        sec.name = "cart"; sec.id = 0; sec.relocatable = true; sec.kind = "RO";
        sec.size = (int64_t)f.bytes.size(); sec.file = "b0.fo"; sec.line = 1;
        o.sections.push_back(sec);

        link::Image img = link::build({o}, scr, pr);
        ok("la banque 0 se lie", img.ok);

        std::map<int, link::Image> perBank;
        perBank[0] = img;
        std::string err;
        std::vector<uint8_t> out = cpr::build(perBank, pr, err);

        ok("pas d'erreur", err.empty());
        ok("taille totale = 8 + 4 + 8 + 16384", out.size() == 8 + 4 + 8 + 16384);
        ok("signature RIFF", out.size() >= 4 && std::string((char *)out.data(), 4) == "RIFF");
        ok("taille RIFF = 4 + (8 + 16384)", out.size() >= 8 &&
               out[4] == 0x0C && out[5] == 0x40 && out[6] == 0x00 && out[7] == 0x00);
        ok("forme AMS!", out.size() >= 12 && std::string((char *)out.data() + 8, 4) == "AMS!");
        ok("chunk cb00", out.size() >= 16 && std::string((char *)out.data() + 12, 4) == "cb00");
        ok("taille du chunk = 0x4000", out.size() >= 20 &&
               out[16] == 0x00 && out[17] == 0x40 && out[18] == 0x00 && out[19] == 0x00);
        ok("octets ecrits recopies", out.size() >= 23 &&
               out[20] == 0xAA && out[21] == 0xBB && out[22] == 0xCC);
        ok("le reste du chunk est a zero", out.size() == 8 + 4 + 8 + 16384 && out[23] == 0x00 &&
               out.back() == 0x00);
    }

    // --- plusieurs banques ; une entrée sans écriture ne produit rien ------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");

        auto bankObj = [](const char *unit, uint8_t byte) {
            asmb::Object o;
            o.name = unit;
            o.sites.push_back({unit, 1});
            asmb::Fragment f;
            f.placed = false;
            f.relocSection = 0;
            f.section = "cart";
            f.bytes = {byte};
            f.prov.assign(f.bytes.size(), 1);
            o.fragments.push_back(f);
            asmb::Section sec;
            sec.name = "cart"; sec.id = 0; sec.relocatable = true; sec.kind = "RO";
            sec.size = 1; sec.file = unit; sec.line = 1;
            o.sections.push_back(sec);
            return o;
        };

        link::Image img0 = link::build({bankObj("b0.fo", 0x11)},
            script::parse("MEMORY_MAP { CONFIG cart_rom.w0<0> { w0 { SECTION cart } } }", "b0.ld"), pr);
        link::Image img3 = link::build({bankObj("b3.fo", 0x33)},
            script::parse("MEMORY_MAP { CONFIG cart_rom.w0<3> { w0 { SECTION cart } } }", "b3.ld"), pr);
        // Banque 5 : liée sans script cart_rom, donc rien n'atterrit sur cet axe.
        link::Image img5 = link::build({bankObj("b5.fo", 0x55)});

        ok("banque 0 se lie", img0.ok);
        ok("banque 3 se lie", img3.ok);
        ok("banque 5 (hors cart_rom) se lie", img5.ok);

        std::map<int, link::Image> perBank;
        perBank[0] = img0;
        perBank[3] = img3;
        perBank[5] = img5;
        std::string err;
        std::vector<uint8_t> out = cpr::build(perBank, pr, err);

        ok("pas d'erreur", err.empty());
        // RIFF(8) + AMS!(4) + 2 chunks (8 + 16384 chacun) — pas de 3e chunk
        ok("taille = deux chunks seulement", out.size() == 8 + 4 + 2 * (8 + 16384));
        ok("premier chunk cb00", out.size() >= 16 && std::string((char *)out.data() + 12, 4) == "cb00");
        ok("octet de la banque 0", out.size() >= 21 && out[20] == 0x11);
        ok("second chunk cb03", out.size() >= 16408 + 4 &&
               std::string((char *)out.data() + 12 + 8 + 16384, 4) == "cb03");
        ok("octet de la banque 3", out.size() >= 12 + 8 + 16384 + 8 + 1 &&
               out[12 + 8 + 16384 + 8] == 0x33);
    }

    // --- ROM physique 19 (> 7) : passe par cart_rom_hi (ROM haute), meme
    // banque crom<n> que cart_rom -> meme chunk cpr, cb13 -------------------
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");

        auto bankObj = [](const char *unit, uint8_t byte) {
            asmb::Object o;
            o.name = unit;
            o.sites.push_back({unit, 1});
            asmb::Fragment f;
            f.placed = false;
            f.relocSection = 0;
            f.section = "cart";
            f.bytes = {byte};
            f.prov.assign(f.bytes.size(), 1);
            o.fragments.push_back(f);
            asmb::Section sec;
            sec.name = "cart"; sec.id = 0; sec.relocatable = true; sec.kind = "RO";
            sec.size = 1; sec.file = unit; sec.line = 1;
            o.sections.push_back(sec);
            return o;
        };

        link::Image img19 = link::build({bankObj("b19.fo", 0x99)},
            script::parse("MEMORY_MAP { CONFIG cart_rom_hi.on<19> { w3 { SECTION cart } } }", "b19.ld"),
            pr);
        ok("banque 19 (cart_rom_hi) se lie", img19.ok);

        std::map<int, link::Image> perBank;
        perBank[19] = img19;
        std::string err;
        std::vector<uint8_t> out = cpr::build(perBank, pr, err);

        ok("pas d'erreur", err.empty());
        ok("un seul chunk", out.size() == 8 + 4 + 8 + 16384);
        ok("chunk cb13 (19 en hexa)",
           out.size() >= 16 && std::string((char *)out.data() + 12, 4) == "cb13");
        ok("octet de la banque 19", out.size() >= 21 && out[20] == 0x99);
    }

    // --- axe present, aucune banque fournie : conteneur vide, pas d'erreur -
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");
        std::map<int, link::Image> perBank;
        std::string err;
        std::vector<uint8_t> out = cpr::build(perBank, pr, err);

        ok("pas d'erreur sur un conteneur vide", err.empty());
        ok("RIFF + AMS! seuls", out.size() == 12);
        ok("taille RIFF = 4", out.size() >= 8 &&
               out[4] == 0x04 && out[5] == 0x00 && out[6] == 0x00 && out[7] == 0x00);
    }

    // --- extractOne() / merge() : le chemin de la CLI, une banque a la fois -
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpcplus"), "cpcplus");

        auto bankObj = [](const char *unit, uint8_t byte) {
            asmb::Object o;
            o.name = unit;
            o.sites.push_back({unit, 1});
            asmb::Fragment f;
            f.placed = false;
            f.relocSection = 0;
            f.section = "cart";
            f.bytes = {byte};
            f.prov.assign(f.bytes.size(), 1);
            o.fragments.push_back(f);
            asmb::Section sec;
            sec.name = "cart"; sec.id = 0; sec.relocatable = true; sec.kind = "RO";
            sec.size = 1; sec.file = unit; sec.line = 1;
            o.sections.push_back(sec);
            return o;
        };
        auto linkBank = [&](const char *unit, uint8_t byte, int n) {
            char scr[256];
            snprintf(scr, sizeof scr,
                "MEMORY_MAP { CONFIG cart_rom.w0<%d> { w0 { SECTION cart } } }", n);
            return link::build({bankObj(unit, byte)}, script::parse(scr, "x.ld"), pr);
        };

        {
            // Une image qui n'ecrit rien sur l'axe : extractOne le dit SANS erreur.
            link::Image img5 = link::build({bankObj("b5.fo", 0x55)});
            std::vector<uint8_t> bytes;
            std::string err;
            ok("rien a extraire (hors axe) : refuse sans erreur",
               !cpr::extractOne(img5, pr, bytes, err) && err.empty());
        }

        std::vector<uint8_t> bank0, bank3;
        std::string err;
        ok("extractOne banque 0", cpr::extractOne(linkBank("b0.fo", 0x11, 0), pr, bank0, err));
        ok("16 Ko, le reste a zero",
           bank0.size() == 16384 && bank0[0] == 0x11 && bank0[1] == 0x00);
        ok("extractOne banque 3", cpr::extractOne(linkBank("b3.fo", 0x33, 3), pr, bank3, err));

        // Premier appel : conteneur vide -> nouveau .cpr avec un seul chunk.
        std::vector<uint8_t> cpr1 = cpr::merge({}, 0, bank0, pr, err);
        ok("premier merge sans erreur", err.empty());
        ok("un chunk cb00", cpr1.size() == 8 + 4 + 8 + 16384 &&
               std::string((char *)cpr1.data() + 12, 4) == "cb00");

        // Second appel : AJOUTE la banque 3 sans toucher la banque 0 deja la.
        std::vector<uint8_t> cpr2 = cpr::merge(cpr1, 3, bank3, pr, err);
        ok("second merge sans erreur", err.empty());
        ok("deux chunks, cb00 garde sa place",
           cpr2.size() == 8 + 4 + 2 * (8 + 16384) &&
           std::string((char *)cpr2.data() + 12, 4) == "cb00" &&
           cpr2[12 + 8] == 0x11 &&
           std::string((char *)cpr2.data() + 12 + 8 + 16384, 4) == "cb03" &&
           cpr2[12 + 8 + 16384 + 8] == 0x33);

        // Troisieme appel : meme id que le premier -> REMPLACE, ne duplique pas.
        std::vector<uint8_t> bankReplaced;
        cpr::extractOne(linkBank("b0b.fo", 0x99, 0), pr, bankReplaced, err);
        std::vector<uint8_t> cpr3 = cpr::merge(cpr2, 0, bankReplaced, pr, err);
        ok("remplacement sans erreur", err.empty());
        ok("toujours deux chunks (pas de doublon)",
           cpr3.size() == 8 + 4 + 2 * (8 + 16384));
        ok("le contenu de cb00 a ete remplace", cpr3[12 + 8] == 0x99);

        // Un fichier qui n'est pas un .cpr : refuse, sans rien produire.
        std::vector<uint8_t> bogus = {0x41, 0x42, 0x43, 0x44};
        std::vector<uint8_t> cpr4 = cpr::merge(bogus, 1, bank3, pr, err);
        ok("conteneur invalide refuse", cpr4.empty() && !err.empty());
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
