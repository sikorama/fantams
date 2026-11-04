// cpr_test.cpp - Tests de l'export cartouche CPR
#include "cpr.h"

#include "link.h"
#include "profile.h"
#include "script.h"

#include <cstdio>
#include <string>
#include <vector>

#include "check.h"
int main() {
    printf("Tests export CPR\n");

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

    return summary();
}
