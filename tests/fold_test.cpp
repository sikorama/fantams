// fold_test.cpp — la source déroulée pliée (ADR 0034).
//
// Le pliage n'a qu'une propriété à tenir, et elle se vérifie sans rien savoir
// de lui : la source déroulée et sa forme pliée s'assemblent aux MÊMES octets,
// avec les mêmes diagnostics aux mêmes lignes. Chaque cas assemble les deux et
// compare. Les assertions sur le TEXTE plié vérifient, elles, que le pliage a
// bien lieu — un pliage qui ne plie rien tiendrait l'invariant sans effort.
#include "asm.h"
#include "link.h"
#include "pp.h"

#include <cstdio>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;

struct Both {
    std::vector<pp::SrcLine> unrolled, folded;
    std::string foldedText;
};

static std::vector<asmb::SourceLine> toAsm(const std::vector<pp::SrcLine> &ls) {
    std::vector<asmb::SourceLine> out;
    for (const auto &l : ls) out.push_back({l.text, l.file, l.line, l.col0});
    return out;
}

static std::string diags(const std::vector<asmb::Diagnostic> &ds) {
    std::string s;
    for (const auto &d : ds) s += d.file + ":" + std::to_string(d.line) + ": " + d.message + "\n";
    return s;
}

// Assemble les deux formes, compare tout ce qu'un auteur peut observer.
static Both same(const char *desc, const std::string &src, const asmb::Constants &given = {}) {
    Both b;
    const pp::Result pre = pp::preprocess(src, "t.asm", nullptr);
    if (!pre.ok) {
        printf("FAIL %s : le préprocesseur refuse le cas\n", desc); ++g_fail; return b;
    }
    b.unrolled = pre.lines;
    b.folded = pp::fold(pre.lines, [&](const std::string &n) { return given.count(n) != 0; });
    for (const auto &l : b.folded) b.foldedText += l.text + "\n";

    const asmb::Object o1 = asmb::assemble(toAsm(b.unrolled), given);
    const asmb::Object o2 = asmb::assemble(toAsm(b.folded), given);
    const link::Image i1 = link::build({o1}), i2 = link::build({o2});
    const link::Flat f1 = link::flatten(i1), f2 = link::flatten(i2);

    std::string why;
    if (o1.ok != o2.ok) why = "ok diffère";
    else if (f1.bytes != f2.bytes || f1.covered != f2.covered) why = "octets ou coverage diffèrent";
    else if (i1.bin != i2.bin || i1.loadAddress != i2.loadAddress) why = "binaire diffère";
    else if (o1.symbols != o2.symbols) why = "table des symboles diffère";
    else if (diags(o1.errors) != diags(o2.errors))
        why = "erreurs diffèrent :\n--- fidèle\n" + diags(o1.errors) + "--- pliée\n" + diags(o2.errors);
    else if (diags(o1.warnings) != diags(o2.warnings))
        why = "avertissements diffèrent :\n--- fidèle\n" + diags(o1.warnings) + "--- pliée\n" + diags(o2.warnings);
    if (why.empty()) ++g_pass;
    else { printf("FAIL %s : %s\n--- texte plié\n%s", desc, why.c_str(), b.foldedText.c_str()); ++g_fail; }
    return b;
}

static void expect(const char *desc, bool cond, const Both &b) {
    if (cond) { ++g_pass; return; }
    printf("FAIL %s\n--- texte plié\n%s", desc, b.foldedText.c_str());
    ++g_fail;
}

static size_t count(const std::string &hay, const std::string &needle) {
    size_t n = 0;
    for (size_t p = hay.find(needle); p != std::string::npos; p = hay.find(needle, p + 1)) ++n;
    return n;
}

int main() {
    // Le cas qui a motivé l'ADR : des affectations par tour, lues seulement par
    // des `if` et des `db` pliés. Elles disparaissent toutes sauf la dernière.
    {
        auto b = same("repeat imbriqué à if",
            "    org #4000\n"
            "    for j = 0 until 2\n"
            "        for x = 0 until 4\n"
            "            ii = (j + (x >> 1)) & 3\n"
            "            w = 0\n"
            "            if ii == 0\n"
            "                db w | %0001, w | %0000\n"
            "            endif\n"
            "            if ii == 1\n"
            "                db w | %0011, w | %1000\n"
            "            endif\n"
            "        endfor\n"
            "    endfor\n");
        expect("les db sont pliés", count(b.foldedText, "db 1,0") > 0 && count(b.foldedText, "|") == 0, b);
        expect("une seule affectation de chaque variable subsiste",
               count(b.foldedText, "ii=") == 1 && count(b.foldedText, "w=") == 1, b);
    }

    // Les flottants : une valeur émise s'écrit en entier, une variable relue
    // garde la précision du double (ADR 0008 : « v = 2.4 » puis « db v*2 » vaut 5).
    {
        auto b = same("table de sinus",
            "    org 0\n"
            "    for t = 0 until 16\n"
            "        db 24 + 20 * sin(2 * 3.14159 * t / 16)\n"
            "    endfor\n");
        expect("sin plié en entiers", count(b.foldedText, "sin") == 0 && count(b.foldedText, "db 24") > 0, b);
    }
    {
        auto b = same("flottant relu", "    org 0\nv = 2.4\n    db v * 2\n    dw v * 1000\n");
        expect("db v*2 plié à 5", count(b.foldedText, "db 5") == 1, b);
    }
    same("flottant non entier conservé", "    org 0\nv = 1 / 3\n    ld a, v * 300\n");

    // Une lecture que le pliage ne touche pas — une instruction — exige toutes
    // les affectations à leur place.
    {
        auto b = same("variable lue par une instruction",
            "    org 0\n"
            "    for k = 0 until 3\n"
            "        w = k * 2\n"
            "        ld a, w\n"
            "    endfor\n");
        expect("toutes les affectations restent", count(b.foldedText, "w=") == 3, b);
    }
    {
        auto b = same("variable lue après la boucle",
            "    org 0\n"
            "    for k = 0 until 3\n"
            "        w = k\n"
            "        db w\n"
            "    endfor\n"
            "    ld a, w\n");
        expect("lue après : toutes restent", count(b.foldedText, "w=") == 3, b);
    }
    // Le repli sur la casse : « W » lit « w », avec un avertissement qui ne doit
    // pas disparaître.
    same("lecture à la casse près", "    org 0\nw = 3\n    db W\nw = 4\n    db W\n");

    // Ce qui ne se plie pas : labels, `$`, locaux, constante définie plus bas,
    // valeur liée à un label.
    same("label et $", "    org #100\nstart:\n    db start & #FF, $ & #FF\n    dw start + 2\n");
    same("constante en avant", "    org 0\n    db K * 2\nK equ 3\n    db K * 2\n");
    {
        auto b = same("variable liée à un label",
            "    org #200\n"
            "    for k = 0 until 2\n"
            "here:\n"
            "        w = 7\n"
            "        w = here + k\n"
            "        dw w\n"
            "    endfor\n");
        (void)b;
    }
    same("locaux", "    org 0\nmain:\n.x = 3\n    db .x\nother:\n.x = 4\n    db .x\n");
    same("equ chaînée", "    org 0\nA1 equ 3\nA2 equ A1 * 2 + 0.5\n    db A2\n    ld a, A2 * 2\n");

    // Les données qui ne sont pas des expressions restent écrites.
    same("chaînes", "    org 0\n    db \"abc\", 0, 'a' - 32\n    db 'hello' - 'a'\n    db 1, 2,\n");

    // Les diagnostics : même erreur, même ligne d'origine.
    same("octet hors limite", "    org 0\nv = 200\n    db v + 100\n");
    same("division par zéro", "    org 0\nz = 0\n    db 4 / z\n");

    // BOUNDARY mesure son bloc à blanc, affectations ignorées : on n'y plie rien.
    same("boundary",
         "    org #3FF0\nn = 4\n    boundary 256\n    ds n\nn = 32\n    ds n\n    end_boundary\n    db n\n");

    // Une constante injectée par le profil se résout chez l'assembleur seul.
    same("constante injectée", "    org 0\nw = __port_x + 1\n    db w\n    db __port_x\n",
         asmb::Constants{{"__port_x", 5}});

    // Une macro et ses labels auto-locaux, pour la forme.
    same("macro",
         "    org 0\n"
         "macro fill n\n"
         "    for q = 0 until {n}\n"
         "        db q * 3\n"
         "    endfor\n"
         "endmacro\n"
         "    fill 3\n"
         "    fill 2\n");

    printf("fold_test: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
