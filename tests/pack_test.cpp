// pack_test.cpp - Tests de l'export DSK (pack::package)
//
// Une image vient du vrai chemin — assembleur puis linker — et le .dsk produit
// est relu comme un émulateur le lirait : en-tête de disquette, répertoire
// AMSDOS, en-tête de 128 octets de chaque fichier. Le lecteur ci-dessous est
// écrit d'après le format, pas d'après pack.cpp.
#include "pack.h"

#include "asm.h"
#include "link.h"
#include "profile.h"
#include "script.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;
static void ok(const char *desc, bool cond) {
    if (cond) ++g_pass;
    else {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %s\n", desc);
    }
}

// --- Lecteur DSK minimal --------------------------------------------------

// Les secteurs d'une disquette, par (piste, ID de secteur).
struct Disk {
    bool valid = false;
    bool extended = false;
    int tracks = 0, sides = 0;
    std::map<std::pair<int, int>, std::vector<uint8_t>> sectors;
};

static Disk readDisk(const std::vector<uint8_t> &d) {
    Disk k;
    if (d.size() < 256) return k;
    const std::string sig(d.begin(), d.begin() + 8);
    if (sig == "EXTENDED") k.extended = true;
    else if (sig != "MV - CPC") return k;
    k.tracks = d[48];
    k.sides = d[49];
    size_t pos = 256;
    for (int t = 0; t < k.tracks * k.sides; ++t) {
        size_t tsize = k.extended ? (size_t)d[52 + t] * 256 : (size_t)(d[50] | d[51] << 8);
        if (tsize == 0) continue;
        if (pos + tsize > d.size()) return k;
        const uint8_t *ti = &d[pos];
        if (std::memcmp(ti, "Track-Info", 10) != 0) return k;
        const int nsec = ti[21];
        size_t data = pos + 256;
        for (int s = 0; s < nsec; ++s) {
            const uint8_t *si = ti + 24 + 8 * s;
            size_t len = k.extended ? (size_t)(si[6] | si[7] << 8) : (size_t)(128u << ti[20]);
            k.sectors[{ti[16], si[2]}] = std::vector<uint8_t>(d.begin() + data, d.begin() + data + len);
            data += len;
        }
        pos += tsize;
    }
    k.valid = true;
    return k;
}

// Format DATA : 9 secteurs &C1..&C9 par piste, blocs de 1 K, répertoire dans
// les blocs 0 et 1.
static std::vector<uint8_t> readBlock(const Disk &k, int block) {
    std::vector<uint8_t> out;
    for (int h = 0; h < 2; ++h) {
        const int idx = block * 2 + h;
        auto it = k.sectors.find({idx / 9, 0xC1 + idx % 9});
        if (it == k.sectors.end()) return {};
        out.insert(out.end(), it->second.begin(), it->second.end());
    }
    return out;
}

struct DirEntry {
    int user;
    std::string name; // « NOM.EXT », espaces retirés
    int extent, records;
    std::vector<int> blocks;
};

static std::vector<DirEntry> readDir(const Disk &k) {
    std::vector<DirEntry> out;
    std::vector<uint8_t> dir = readBlock(k, 0);
    std::vector<uint8_t> b1 = readBlock(k, 1);
    dir.insert(dir.end(), b1.begin(), b1.end());
    for (size_t e = 0; e + 32 <= dir.size(); e += 32) {
        const uint8_t *p = &dir[e];
        if (p[0] == 0xE5) continue;
        DirEntry de;
        de.user = p[0];
        std::string n, x;
        for (int i = 1; i <= 8; ++i)
            if (p[i] != ' ') n += (char)(p[i] & 0x7F);
        for (int i = 9; i <= 11; ++i)
            if (p[i] != ' ') x += (char)(p[i] & 0x7F);
        de.name = n + "." + x;
        de.extent = p[12];
        de.records = p[15];
        for (int i = 16; i < 32; ++i)
            if (p[i]) de.blocks.push_back(p[i]);
        out.push_back(de);
    }
    return out;
}

// Le contenu d'un fichier, en-tête AMSDOS compris ; vide s'il n'existe pas.
static std::vector<uint8_t> readFile(const Disk &k, const std::string &name) {
    std::map<int, DirEntry> extents;
    for (const DirEntry &de : readDir(k))
        if (de.user == 0 && de.name == name) extents[de.extent] = de;
    std::vector<uint8_t> out;
    for (auto &[ex, de] : extents) {
        std::vector<uint8_t> part;
        for (int b : de.blocks) {
            std::vector<uint8_t> blk = readBlock(k, b);
            part.insert(part.end(), blk.begin(), blk.end());
        }
        part.resize((size_t)de.records * 128);
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

static std::vector<std::string> fileNames(const Disk &k) {
    std::vector<std::string> out;
    for (const DirEntry &de : readDir(k))
        if (de.extent == 0) out.push_back(de.name);
    return out;
}

struct Header {
    bool checksumOk = false;
    int type = -1, load = -1, entry = -1, length = -1;
    std::string name;
};

static Header readHeader(const std::vector<uint8_t> &f) {
    Header h;
    if (f.size() < 128) return h;
    unsigned sum = 0;
    for (int i = 0; i <= 66; ++i) sum += f[i];
    h.checksumOk = (sum & 0xFFFF) == (unsigned)(f[67] | f[68] << 8);
    h.type = f[18];
    h.load = f[21] | f[22] << 8;
    h.length = f[64] | f[65] << 8 | f[66] << 16;
    h.entry = f[26] | f[27] << 8;
    for (int i = 1; i <= 8; ++i)
        if (f[i] != ' ') h.name += (char)f[i];
    return h;
}

// Les octets du programme, après l'en-tête et dans la longueur qu'il annonce.
static std::vector<uint8_t> payload(const std::vector<uint8_t> &f) {
    const Header h = readHeader(f);
    if (h.length < 0 || f.size() < 128 + (size_t)h.length) return {};
    return std::vector<uint8_t>(f.begin() + 128, f.begin() + 128 + h.length);
}

// --- Retouches d'une image produite, pour fabriquer des disquettes d'autres
// outils ---------------------------------------------------------------------

// La même disquette au format standard (MV - CPCEMU) : une taille de piste
// commune dans l'en-tête, pas de longueur par secteur.
static std::vector<uint8_t> toStandard(std::vector<uint8_t> d) {
    const char *sig = "MV - CPCEMU Disk-File\r\nDisk-Info\r\n";
    std::memset(d.data(), 0, 52 + 204);
    std::memcpy(d.data(), sig, std::strlen(sig));
    d[48] = 40;
    d[49] = 1;
    d[50] = 0x00;
    d[51] = 0x13; // 256 + 9 x 512
    for (size_t pos = 256; pos + 256 <= d.size(); pos += 0x1300)
        for (int s = 0; s < 9; ++s) {
            d[pos + 24 + 8 * s + 6] = 0;
            d[pos + 24 + 8 * s + 7] = 0;
        }
    return d;
}

// Remplace dans l'image les 11 octets NOM+EXT d'une entrée de répertoire.
static void renameEntry(std::vector<uint8_t> &d, const char *from11, const char *to11) {
    for (size_t i = 0; i + 11 <= d.size(); ++i)
        if (std::memcmp(&d[i], from11, 11) == 0) {
            std::memcpy(&d[i], to11, 11);
            return;
        }
}

static link::Image linkText(const std::string &src, const char *file) {
    return link::build({asmb::assembleText(src, file)});
}

int main() {
    printf("Tests export DSK\n");

    // --- un seul org : un fichier nommé d'après le source -----------------
    {
        const link::Image img = linkText("        org 0x8000\n"
                                         "        run start\n"
                                         "start:  ld a,1\n"
                                         "        ret\n",
                                         "demo.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "examples/demo.asm";
        const pack::Result r = pack::package(img, dsk);

        ok("pas d'erreur", r.ok && r.errors.empty());
        ok("un seul artefact", r.artifacts.size() == 1);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("une disquette EXTENDED", k.valid && k.extended);
        ok("40 pistes, une face", k.tracks == 40 && k.sides == 1);
        ok("un seul fichier, DEMO.BIN",
           fileNames(k) == std::vector<std::string>{"DEMO.BIN"});
        const std::vector<uint8_t> f = readFile(k, "DEMO.BIN");
        const Header h = readHeader(f);
        ok("somme de contrôle juste", h.checksumOk);
        ok("type binaire", h.type == 2);
        ok("chargé en &8000", h.load == 0x8000);
        ok("entrée en &8000", h.entry == 0x8000);
        ok("le nom dans l'en-tête", h.name == "DEMO");
        ok("les octets du programme",
           payload(f) == std::vector<uint8_t>({0x3E, 0x01, 0xC9}));
    }

    // --- &4000..&9000 d'un seul tenant : un seul fichier ------------------
    // Le linker coupe ce programme en deux blocs (banques 1 et 2) : un bloc ne
    // franchit jamais une frontière de 16 K.
    {
        const link::Image img = linkText("        org 0x4000\n"
                                         "        ds 0x5000, 0x77\n",
                                         "long.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "long.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("un seul fichier, LONG.BIN", fileNames(k) == std::vector<std::string>{"LONG.BIN"});
        const std::vector<uint8_t> f = readFile(k, "LONG.BIN");
        ok("chargé en &4000", readHeader(f).load == 0x4000);
        const std::vector<uint8_t> body = payload(f);
        ok("&5000 octets", body.size() == 0x5000);
        ok("le dernier octet est celui de &8FFF", body.size() == 0x5000 && body.back() == 0x77);
    }

    // --- deux org disjoints : deux fichiers aux noms dérivés ---------------
    {
        const link::Image img = linkText("        org 0x4000\n"
                                         "        db 1, 2\n"
                                         "        org 0x8000\n"
                                         "        run start\n"
                                         "        db 3\n"
                                         "start:  ret\n",
                                         "two.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "two.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("deux fichiers, B01_4000.BIN et B02_8000.BIN",
           fileNames(k) == std::vector<std::string>({"B01_4000.BIN", "B02_8000.BIN"}));
        const Header lo = readHeader(readFile(k, "B01_4000.BIN"));
        const Header hi = readHeader(readFile(k, "B02_8000.BIN"));
        ok("B01_4000 chargé en &4000", lo.load == 0x4000);
        ok("B02_8000 chargé en &8000", hi.load == 0x8000);
        ok("le nom dérivé dans l'en-tête", hi.name == "B02_8000");
        ok("B01_4000 ne contient pas le run : entrée à 0", lo.entry == 0);
        ok("B02_8000 contient le run : entrée en &8001", hi.entry == 0x8001);
        ok("aucun avertissement", r.warnings.empty());
    }

    // --- un bloc en banque d'extension : son propre fichier ----------------
    // `audio` vit dans la première banque étendue, vue en &4000 : il touche
    // `main` en adresse, mais pas en banque, et ne se fusionne donc pas.
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        const script::Script scr = script::parse(
            "TARGET cpc6128\n"
            "MEMORY_MAP {\n"
            "    CONFIG linear { w0 { SECTION main } }\n"
            "    CONFIG ext_w1<1> { w1 { SECTION audio } }\n"
            "}\n",
            "ext.ld");
        const asmb::Object o = asmb::assembleText("        SECTION main, \"ro\"\n"
                                                  "        run start\n"
                                                  "start:  ret\n"
                                                  "        ds 0x3FFF\n"
                                                  "        SECTION audio, \"ro\"\n"
                                                  "        db 0x55, 0x66\n",
                                                  "ext.asm");
        const link::Image img = link::build({o}, scr, pr);
        ok("le source se lie", o.ok && img.ok);
        int extBank = -1;
        for (int b : img.banksWritten)
            if (b >= 4) extBank = b;
        ok("une banque d'extension est écrite", extBank >= 4);

        pack::Dsk dsk;
        dsk.program = "ext.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        char extName[16];
        std::snprintf(extName, sizeof extName, "B%02X_4000.BIN", extBank);
        ok("deux fichiers : main et la banque d'extension",
           fileNames(k) == std::vector<std::string>({"B00_0000.BIN", extName}));
        const std::vector<uint8_t> f = readFile(k, extName);
        ok("la banque d'extension est chargée en &4000", readHeader(f).load == 0x4000);
        ok("ses octets", payload(f) == std::vector<uint8_t>({0x55, 0x66}));
        ok("pas d'entrée sur la banque d'extension", readHeader(f).entry == 0);
    }

    // --- run en banque d'extension : l'entrée suit la banque, pas l'adresse -
    // `main` en &4000 dans les 64 K de base et `audio` en &4000 dans la banque
    // d'extension couvrent tous deux l'adresse du `run` : seul le fichier de la
    // banque qui porte le label reçoit l'entrée.
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        const script::Script scr = script::parse(
            "TARGET cpc6128\n"
            "MEMORY_MAP {\n"
            "    CONFIG linear { w1 { SECTION main } }\n"
            "    CONFIG ext_w1<1> { w1 { SECTION audio } }\n"
            "}\n",
            "extrun.ld");
        const asmb::Object o = asmb::assembleText("        SECTION main, \"ro\"\n"
                                                  "        run play\n"
                                                  "        db 1, 2, 3, 4\n"
                                                  "        SECTION audio, \"ro\"\n"
                                                  "        nop\n"
                                                  "play:   ret\n",
                                                  "extrun.asm");
        const link::Image img = link::build({o}, scr, pr);
        ok("le source se lie", o.ok && img.ok);
        int extBank = -1;
        for (int b : img.banksWritten)
            if (b >= 4) extBank = b;
        pack::Dsk dsk;
        dsk.program = "extrun.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        char extName[16];
        std::snprintf(extName, sizeof extName, "B%02X_4000.BIN", extBank);
        ok("main, dans les 64 K de base : entrée à 0",
           readHeader(readFile(k, "B01_4000.BIN")).entry == 0);
        ok("la banque d'extension porte le run : entrée en &4001",
           readHeader(readFile(k, extName)).entry == 0x4001);
        ok("aucun avertissement", r.warnings.empty());
    }

    // --- run hors de tout morceau : avertissement, entrées à 0 -------------
    // Le code est rangé en &4000 et assemblé pour &8000 : le `run` est une
    // adresse logique, que le fichier rangé en &4000 ne contient pas.
    {
        const link::Image img = linkText("        org 0x8000, 0x4000\n"
                                         "        run start\n"
                                         "start:  ret\n",
                                         "moved.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "moved.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        const Header h = readHeader(readFile(k, "MOVED.BIN"));
        ok("chargé à l'adresse de rangement &4000", h.load == 0x4000);
        ok("entrée à 0", h.entry == 0);
        ok("pas une erreur", r.ok && r.errors.empty());
        ok("un avertissement qui nomme &8000",
           r.warnings.size() == 1 && r.warnings[0].message.find("8000") != std::string::npos);
    }

    // --- nom du source filtré ; vide, il cède au nom dérivé ----------------
    {
        const link::Image img = linkText("        org 0x8000\n"
                                         "        db 1\n",
                                         "x.asm");
        pack::Dsk dsk;
        dsk.program = "src/my prog!.v2.asm";
        pack::Result r = pack::package(img, dsk);
        Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("majuscules, caractères AMSDOS seuls, 8 au plus : MYPROGV2.BIN",
           fileNames(k) == std::vector<std::string>{"MYPROGV2.BIN"});

        dsk.program = "!!!.asm";
        r = pack::package(img, dsk);
        k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("aucun caractère valable : le nom dérivé B02_8000.BIN",
           fileNames(k) == std::vector<std::string>{"B02_8000.BIN"});
    }

    // --- trous réservés : écrits à zéro, sans rognage ----------------------
    {
        const link::Image img = linkText("        org 0x8000\n"
                                         "        db 1\n"
                                         "        ds 2\n"
                                         "        db 2\n"
                                         "        ds 3\n",
                                         "holes.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "holes.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("les trous sont à zéro, y compris en fin de morceau",
           payload(readFile(k, "HOLES.BIN")) ==
               std::vector<uint8_t>({0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}));
    }

    // --- fusion : les autres fichiers restent, le même nom est remplacé ----
    {
        pack::Dsk first;
        first.program = "other.asm";
        const pack::Result r1 =
            pack::package(linkText("        org 0x1000\n        db 0xAA, 0xBB\n", "other.asm"), first);
        pack::Dsk second;
        second.program = "demo.asm";
        second.existing = r1.artifacts.empty() ? std::vector<uint8_t>() : r1.artifacts[0].bytes;
        const pack::Result r2 =
            pack::package(linkText("        org 0x8000\n        db 1\n", "demo.asm"), second);
        ok("fusion sans erreur", r2.ok && r2.errors.empty());
        pack::Dsk third;
        third.program = "demo.asm";
        third.existing = r2.artifacts.empty() ? std::vector<uint8_t>() : r2.artifacts[0].bytes;
        const pack::Result r3 =
            pack::package(linkText("        org 0x9000\n        db 2, 3\n", "demo.asm"), third);
        const Disk k = readDisk(r3.artifacts.empty() ? std::vector<uint8_t>() : r3.artifacts[0].bytes);
        std::vector<std::string> names = fileNames(k);
        std::sort(names.begin(), names.end());
        ok("deux fichiers : DEMO.BIN une seule fois, et OTHER.BIN",
           names == std::vector<std::string>({"DEMO.BIN", "OTHER.BIN"}));
        ok("OTHER.BIN conservé à l'octet",
           payload(readFile(k, "OTHER.BIN")) == std::vector<uint8_t>({0xAA, 0xBB}));
        const std::vector<uint8_t> f = readFile(k, "DEMO.BIN");
        ok("DEMO.BIN remplacé par le nouveau build",
           readHeader(f).load == 0x9000 && payload(f) == std::vector<uint8_t>({2, 3}));
    }

    // --- fusion avec une disquette standard ; nom comparé sans la casse ----
    {
        pack::Dsk first;
        first.program = "other.asm";
        std::vector<uint8_t> d =
            pack::package(linkText("        org 0x1000\n        db 0xAA\n", "o.asm"), first).artifacts[0].bytes;
        first.program = "demo.asm";
        first.existing = d;
        d = pack::package(linkText("        org 0x8000\n        db 1\n", "d.asm"), first).artifacts[0].bytes;
        // Un autre outil a écrit « demo.bin » en minuscules, au format standard.
        renameEntry(d, "DEMO    BIN", "demo    bin");
        d = toStandard(d);
        ok("le lecteur du test lit la disquette standard", readDisk(d).valid && !readDisk(d).extended);

        pack::Dsk again;
        again.program = "demo.asm";
        again.existing = d;
        const pack::Result r =
            pack::package(linkText("        org 0x9000\n        db 2\n", "d.asm"), again);
        ok("disquette standard acceptée", r.ok && r.errors.empty());
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("réécrite en EXTENDED", k.valid && k.extended);
        std::vector<std::string> names = fileNames(k);
        std::sort(names.begin(), names.end());
        ok("demo.bin remplacé par DEMO.BIN, OTHER.BIN conservé",
           names == std::vector<std::string>({"DEMO.BIN", "OTHER.BIN"}));
        ok("OTHER.BIN conservé à l'octet",
           payload(readFile(k, "OTHER.BIN")) == std::vector<uint8_t>({0xAA}));
    }

    // --- disquette existante hors format DATA : refus, aucun artefact -----
    {
        pack::Dsk first;
        first.program = "other.asm";
        const std::vector<uint8_t> good =
            pack::package(linkText("        org 0x1000\n        db 0xAA\n", "o.asm"), first).artifacts[0].bytes;
        const link::Image img = linkText("        org 0x8000\n        db 1\n", "d.asm");

        std::vector<uint8_t> twoSides = good;
        twoSides[49] = 2;
        pack::Dsk dsk;
        dsk.program = "demo.asm";
        dsk.existing = twoSides;
        pack::Result r = pack::package(img, dsk);
        ok("double face refusée", !r.ok && !r.errors.empty() && r.artifacts.empty());

        // Format SYSTEM : secteurs &41..&49.
        std::vector<uint8_t> system = good;
        for (size_t pos = 256; pos + 256 <= system.size(); pos += 0x1300)
            for (int s = 0; s < 9; ++s) system[pos + 24 + 8 * s + 2] -= 0x80;
        dsk.existing = system;
        r = pack::package(img, dsk);
        ok("format SYSTEM refusé", !r.ok && !r.errors.empty() && r.artifacts.empty());

        dsk.existing = std::vector<uint8_t>(300, 0x42);
        r = pack::package(img, dsk);
        ok("ce qui n'est pas une disquette est refusé", !r.ok && !r.errors.empty() && r.artifacts.empty());
    }

    // --- disquette pleine, en place : erreur, aucun artefact ---------------
    {
        // Trois fichiers de 56 K + en-tête : 57 blocs chacun, 171 sur 178.
        std::vector<uint8_t> d;
        bool built = true;
        for (const char *n : {"a.asm", "b.asm", "c.asm"}) {
            pack::Dsk dsk;
            dsk.program = n;
            dsk.existing = d;
            const pack::Result r =
                pack::package(linkText("        org 0x1000\n        ds 0xE000, 1\n", n), dsk);
            built = built && r.ok;
            if (r.ok) d = r.artifacts[0].bytes;
        }
        ok("171 blocs sur 178 tiennent", built);
        pack::Dsk dsk;
        dsk.program = "d.asm";
        dsk.existing = d;
        // 8 K + en-tête : 9 blocs, il en reste 7.
        const pack::Result r = pack::package(linkText("        org 0x1000\n        ds 0x2000, 1\n", "d.asm"), dsk);
        ok("disquette pleine : erreur", !r.ok && !r.errors.empty());
        ok("disquette pleine : aucun artefact", r.artifacts.empty());
    }

    // --- répertoire plein : erreur à la 65e entrée --------------------------
    {
        std::vector<uint8_t> d;
        bool built = true;
        for (int i = 0; i < 64; ++i) {
            pack::Dsk dsk;
            dsk.program = "F" + std::to_string(i) + ".asm";
            dsk.existing = d;
            const pack::Result r = pack::package(linkText("        org 0x1000\n        db 1\n", "f.asm"), dsk);
            built = built && r.ok;
            if (r.ok) d = r.artifacts[0].bytes;
        }
        ok("64 fichiers tiennent", built);
        pack::Dsk dsk;
        dsk.program = "F64.asm";
        dsk.existing = d;
        const pack::Result r = pack::package(linkText("        org 0x1000\n        db 1\n", "f.asm"), dsk);
        ok("65e fichier : erreur", !r.ok && !r.errors.empty() && r.artifacts.empty());
    }

    // --- deux sections contiguës dans une banque d'extension : un fichier ---
    {
        const profile::Profile pr = profile::parse(profile::builtin("cpc6128"), "cpc6128");
        const script::Script scr = script::parse(
            "TARGET cpc6128\n"
            "MEMORY_MAP {\n"
            "    CONFIG linear { w0 { SECTION main } }\n"
            "    CONFIG ext_w1<1> { w1 { SECTION audio  SECTION packed } }\n"
            "}\n",
            "ext2.ld");
        const asmb::Object o = asmb::assembleText("        SECTION main, \"ro\"\n"
                                                  "        ret\n"
                                                  "        SECTION audio, \"ro\"\n"
                                                  "        db 0x11, 0x22\n"
                                                  "        SECTION packed, \"ro\"\n"
                                                  "        db 0x33\n",
                                                  "ext2.asm");
        const link::Image img = link::build({o}, scr, pr);
        ok("le source se lie", o.ok && img.ok);
        pack::Dsk dsk;
        dsk.program = "ext2.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        std::vector<std::string> names = fileNames(k);
        ok("deux fichiers : main, et la banque d'extension en un seul", names.size() == 2);
        std::string extName;
        for (const std::string &n : names)
            if (n != "B00_0000.BIN") extName = n;
        ok("audio et packed dans le même fichier",
           payload(readFile(k, extName)) == std::vector<uint8_t>({0x11, 0x22, 0x33}));
    }

    // --- blocs contigus posés dans le désordre : un fichier ----------------
    {
        const link::Image img = linkText("        org 0x8002\n"
                                         "        db 3\n"
                                         "        org 0x8000\n"
                                         "        db 1, 2\n",
                                         "rev.asm");
        ok("le source se lie", img.ok);
        pack::Dsk dsk;
        dsk.program = "rev.asm";
        const pack::Result r = pack::package(img, dsk);
        const Disk k = readDisk(r.artifacts.empty() ? std::vector<uint8_t>() : r.artifacts[0].bytes);
        ok("un seul fichier, REV.BIN", fileNames(k) == std::vector<std::string>{"REV.BIN"});
        const std::vector<uint8_t> f = readFile(k, "REV.BIN");
        ok("chargé en &8000, octets dans l'ordre des adresses",
           readHeader(f).load == 0x8000 && payload(f) == std::vector<uint8_t>({1, 2, 3}));
    }

    printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
