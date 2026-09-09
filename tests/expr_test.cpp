// expr_test.cpp - Tests de l'évaluateur d'expressions (fantams)
#include "expr.h"

#include <cstdio>
#include <map>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;

// Le resolveur de test rend des sections FACTICES : deux entiers opaques, qui
// suffisent a tout ce que l'evaluateur sait d'une section — « la meme » ou
// « pas la meme ». C'est ce qui permet de tester l'affinite entiere sans
// assembler une ligne.
static const expr::SectionId S1 = 1, S2 = 2;

static std::map<std::string, expr::Value> g_syms;
static expr::Resolver resolver = [](const std::string &n, expr::Value &out) -> bool {
    auto it = g_syms.find(n);
    if (it == g_syms.end()) return false;
    out = it->second; return true;
};

// Un label d'une section relocalisable : sa base plus son offset.
static expr::Value reloc(expr::SectionId sec, double offset) {
    expr::Value v; v.section = sec; v.coeff = 1; v.real = offset; return v;
}

static void chk(const char *desc, const std::string &text, int64_t expected) {
    expr::Result r = expr::eval(text, resolver);
    if (!r.ok || r.value != expected) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s attendu %lld obtenu %lld%s\n",
               desc, (long long)expected, (long long)r.value,
               r.ok ? "" : (" err: " + r.error).c_str());
    } else ++g_pass;
}

static void chkErr(const char *desc, const std::string &text) {
    expr::Result r = expr::eval(text, resolver);
    if (r.ok) { ++g_fail; printf("  \033[31mFAIL\033[0m %-40s aurait dû échouer (= %lld)\n", desc, (long long)r.value); }
    else ++g_pass;
}

// Un refus ET ce qu'il enseigne : le message doit porter `needle`. Sans cela,
// « label >> 8 est refuse » serait vrai sans dire quoi ecrire a la place.
static void chkErrSays(const char *desc, const std::string &text, const std::string &needle) {
    expr::Result r = expr::eval(text, resolver);
    if (r.ok) { ++g_fail; printf("  \033[31mFAIL\033[0m %-40s aurait dû échouer (= %lld)\n", desc, (long long)r.value); return; }
    if (r.error.find(needle) == std::string::npos) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s le message ne dit pas \"%s\" : %s\n",
               desc, needle.c_str(), r.error.c_str());
    } else ++g_pass;
}

// Une valeur ABSOLUE : coefficient nul, et la valeur attendue.
static void chkAbs(const char *desc, const std::string &text, int64_t expected) {
    expr::Result r = expr::eval(text, resolver);
    if (!r.ok || r.value != expected || !r.absolute()) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s attendu %lld absolu, obtenu %lld coeff %d%s\n",
               desc, (long long)expected, (long long)r.value, r.coeff,
               r.ok ? "" : (" err: " + r.error).c_str());
    } else ++g_pass;
}

// Une valeur RELOCALISABLE : sa section, son coefficient, son addend, son octet.
static void chkReloc(const char *desc, const std::string &text, expr::SectionId sec,
                     int coeff, int64_t addend, expr::Byte byte = expr::Byte::Whole) {
    expr::Result r = expr::eval(text, resolver);
    if (!r.ok || r.section != sec || r.coeff != coeff || r.value != addend || r.byte != byte) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s attendu (sec %d, coeff %d, addend %lld, octet %d), "
               "obtenu (sec %d, coeff %d, addend %lld, octet %d)%s\n",
               desc, sec, coeff, (long long)addend, (int)byte,
               r.section, r.coeff, (long long)r.value, (int)r.byte,
               r.ok ? "" : (" err: " + r.error).c_str());
    } else ++g_pass;
}

int main() {
    printf("Tests évaluateur d'expressions\n");
    g_syms["start"] = expr::Value{0x8000, expr::NoSection, 0, expr::Byte::Whole};

    // --- de base (préservé du comportement historique) ---
    // --- puissance (ADR 0008) ----------------------------------------------
    chk("puissance", "2**3", 8);
    chk("puissance, grands nombres", "2**10", 1024);
    // Plus liante que l'unaire : -(2**2), comme partout ailleurs.
    chk("l'unaire moins s'applique APRÈS", "-2**2", -4);
    // Associative à DROITE : 2**(3**2) = 2**9, et non (2**3)**2 = 64.
    chk("associative à droite", "2**3**2", 512);
    chk("exposant négatif", "4**-1 * 4", 1);
    chk("'*' simple n'est pas touché", "2*3", 6);
    chk("'^' reste le ou exclusif", "3^2", 1);
    chk("espaces autour de **", "2 ** 3", 8);
    chkErr("'***' n'est pas un opérateur", "2***3");

    chk("entier décimal", "42", 42);
    chk("hexa #", "#1234", 0x1234);
    chk("hexa 0x", "0x1234", 0x1234);
    chk("binaire %", "%1010", 10);
    chk("caractère", "'A'", 65);
    chk("addition", "1+2*3", 7);
    chk("parenthèses", "(1+2)*3", 9);
    chk("bit or", "%1100 | %0011", 15);
    chk("bit and", "%1100 & %1010", 8);
    chk("shift", "1 << 4", 16);
    chk("modulo", "7 % 3", 1);
    chk("négatif", "-5+3", -2);
    chk("not bit à bit", "~0 & 0xFF", 255);
    chk("symbole", "start+2", 0x8002);
    chkErr("symbole inconnu", "nope");
    chkErr("division par zéro", "1/0");

    // --- rondage "half up" final (comme l'assembleur de référence : db 7/2 -> 4, db -7/2 -> -3) ---
    chk("division exacte", "6/2", 3);
    chk("arrondi positif .5 -> +1", "7/2", 4);
    chk("arrondi négatif .5 -> vers 0", "-7/2", -3);
    chk("arrondi .4 -> troncature", "9/4", 2);   // 2.25 -> 2
    chk("arrondi .6 -> +1", "11/4", 3);          // 2.75 -> 3

    // --- littéraux flottants ---
    chk("flottant simple", "0.5*10", 5);
    chk("flottant dans expression", "1 + 0.2*10", 3); // 1+2.0=3.0

    // --- fonctions : sin/cos (RADIANS, ADR 0021) / abs / hi / lo ---
    chk("sin(0)", "sin(0)", 0);
    chk("sin(1)*1000", "sin(1)*1000", 841);
    chk("cos(0)*100", "cos(0)*100", 100);
    chk("cos(1)*1000", "cos(1)*1000", 540);
    chk("sin(pi) ~ 0", "sin(3.14159265)*1000", 0);
    chk("cos(pi) = -1", "cos(3.14159265)*100", -100);
    chk("sin(1)*1000 mod 256 (comme db)", "(sin(1)*1000) & 255", 73);
    // 90 est un angle en radians, pas un quart de tour : sin(90 rad) = 0.894
    chk("l'argument n'est pas en degrés", "sin(90)*1000", 894);
    chk("abs négatif", "abs(-5)", 5);
    chk("abs positif", "abs(5)", 5);
    chk("hi", "hi(#1234)", 0x12);
    chk("lo", "lo(#1234)", 0x34);
    chk("expression complète", "20 + 10 * sin(1)", 28);
    // division réelle avant sin (pas de troncature entière intermédiaire) :
    // (2*pi)/256 = 0.0245 rad -> sin -> *1000 -> round -> 25
    chk("division flottante avant sin", "sin((6.28318530*1)/256)*1000", 25);

    // --- Litteraux de chaine (ADR 0010) ---
    // Une expression rend un NOMBRE : seul un litteral d'un octet en a un, et
    // les deux delimiteurs sont synonymes.
    chk("litteral simple quote", "'A'", 65);
    chk("litteral double quote", "\"A\"", 65);
    chk("litteral dans une arithmetique", "'a'+1", 98);
    chk("les deux delimiteurs se melangent", "'z'-\"a\"", 25);
    chk("le delimiteur oppose est du contenu", "'\"'", 34);
    chk("l'autre sens", "\"'\"", 39);
    chkErr("deux octets n'ont pas de valeur", "'ab'");
    chkErr("deux octets, delimiteur double", "\"ab\"");
    chkErr("litteral vide n'a pas de valeur", "''");
    chkErr("litteral vide, delimiteur double", "\"\"");
    chkErr("litteral non termine", "'ab");

    // --- Valeurs relocalisables (etage B, D2 / D3) --------------------------
    // Toute l'affinite se teste ICI, avec un resolveur rendant des sections
    // factices : pas un octet d'assembleur n'est necessaire.
    printf("\n  valeur affine\n");
    g_syms["debut"] = reloc(S1, 0);
    g_syms["fin"]   = reloc(S1, 0x40);
    g_syms["ailleurs"] = reloc(S2, 0x10);

    // Un label seul est relocalisable ; l'addend est son offset dans la section.
    chkReloc("un label seul est relocalisable", "debut", S1, 1, 0);
    chkReloc("un label plus loin dans sa section", "fin", S1, 1, 0x40);
    chkReloc("une adresse se deplace d'un nombre", "debut + 4", S1, 1, 4);
    chkReloc("dans l'autre sens", "fin - 4", S1, 1, 0x3C);
    chkReloc("le nombre peut venir en premier", "4 + debut", S1, 1, 4);
    chkReloc("a travers des parentheses", "(debut) + 1", S1, 1, 1);
    chkReloc("l'unaire plus ne change rien", "+debut", S1, 1, 0);

    // Les coefficients s'annulent : mesurer une table reste un NOMBRE.
    chkAbs("fin - debut, meme section, est absolu", "fin - debut", 0x40);
    chkAbs("et il se calcule ensuite", "(fin - debut) * 2", 0x80);
    chkAbs("meme decale", "(fin + 2) - (debut + 1)", 0x41);

    // Ce qui n'a pas de sens est refuse, jamais calcule au hasard.
    chkErr("label * 2 est refuse", "debut * 2");
    chkErr("label / 2 est refuse", "debut / 2");
    chkErr("label + label est refuse", "debut + fin");
    chkErr("label ** 2 est refuse", "debut ** 2");
    chkErr("une comparaison est refusee", "debut < fin");
    chkErr("une fonction reelle est refusee", "sin(debut)");
    chkErr("min() est refuse", "min(debut, 4)");
    chkErrSays("deux sections differentes", "debut - ailleurs", "different sections");
    chkErrSays("l'addition aussi", "debut + ailleurs", "different sections");
    chkErrSays("une base citee deux fois", "debut + debut", "twice");
    chkErrSays("une partie fractionnaire", "debut + 0.5", "fractional");

    // L'unaire moins garde l'affinite ; c'est ce qui permet a `a - b` de
    // s'annuler. Un -1 tout seul n'est pas emettable, mais ce n'est pas a
    // l'evaluateur d'en juger.
    chkReloc("l'unaire moins donne un coefficient -1", "-debut", S1, -1, 0);
    chkAbs("et il s'annule contre un +1", "fin + -debut", 0x40);

    // --- high() / low() -----------------------------------------------------
    // Les seules fonctions qui acceptent une adresse inconnue : la reponse au
    // refus de `>> 8` et de `& 255`.
    chkReloc("high() d'une adresse", "high(debut)", S1, 1, 0, expr::Byte::High);
    chkReloc("low() d'une adresse", "low(debut)", S1, 1, 0, expr::Byte::Low);
    chkReloc("l'offset va DEDANS", "high(debut + 3)", S1, 1, 3, expr::Byte::High);
    chkReloc("hi() en est une graphie", "hi(debut)", S1, 1, 0, expr::Byte::High);
    chkReloc("lo() aussi", "lo(debut)", S1, 1, 0, expr::Byte::Low);
    chkErrSays("rien ne se calcule apres", "high(debut) + 1", "high(label + 1)");
    chkErrSays("ni deux fois de suite", "low(high(debut))", "high(label + 1)");
    chkErr("high() d'une negation est refuse", "high(-debut)");

    // Sur une valeur ABSOLUE, les quatre calculent comme avant.
    chkAbs("high() absolu", "high(#1234)", 0x12);
    chkAbs("low() absolu", "low(#1234)", 0x34);
    chkAbs("high() se calcule ensuite", "high(#1234) + 1", 0x13);

    // --- bankof() : une QUESTION, non un placement --------------------------
    // Elle rend l'emplacement de rangement de la SECTION, ce que seul le linker
    // connait. La graphie est distincte de `BANK`, qui reste un mot REFUSE parce
    // qu'il nommait un placement que la source ne fait pas ; et elle se lit a
    // cote de `sizeof()`.
    chkReloc("bankof() d'une adresse", "bankof(debut)", S1, 1, 0, expr::Byte::Bank);
    // L'offset ne survit PAS : la banque d'une section ne bouge pas avec un
    // decalage, et laisser un addend traîner ferait croire qu'un `+1` peut
    // changer de banque.
    chkReloc("l'offset ne survit pas", "bankof(debut + 3)", S1, 1, 0, expr::Byte::Bank);
    chkErr("bankof() d'une valeur absolue est refuse", "bankof(#1234)");
    chkErrSays("et le refus dit pourquoi", "bankof(#1234)", "no linker decision");
    chkErr("rien ne se calcule apres", "bankof(debut) + 1");
    chkErr("ni deux fois de suite", "bankof(high(debut))");

    // --- les deux idiomes refuses NOMMENT leur remplacant -------------------
    chkErrSays("label >> 8 nomme high()", "debut >> 8", "high()");
    chkErrSays("label & 255 nomme low()", "debut & 255", "low()");
    chkErrSays("shr aussi", "debut shr 8", "high()");
    chkErrSays("and aussi", "debut and 255", "low()");
    // Les memes formes restent legales sur un nombre.
    chkAbs("start >> 8 reste legal", "start >> 8", 0x80);
    chkAbs("start & 255 reste legal", "start & 255", 0);

    // --- opcode() : un simple point d'injection, jamais une dépendance directe
    // vers parser/z80 (ADR 0031) ----------------------------------------------
    {
        // Sans hook, opcode() échoue en le disant : c'est le chemin du
        // préprocesseur pur, qui ne peut jamais joindre parser/z80.
        expr::Result r = expr::eval("opcode(\"ld a,n\",0)", resolver, nullptr);
        bool named = !r.ok && r.error.find("not available") != std::string::npos;
        if (!named) { ++g_fail; printf("  \033[31mFAIL\033[0m opcode() sans hook nomme l'absence : %s\n", r.error.c_str()); }
        else ++g_pass;
    }
    {
        // Avec un hook factice, l'appel lui est transmis tel quel : le texte de
        // l'instruction, et index/len — par défaut 0 et 1 quand omis.
        struct Call { std::string text; int index; int len; };
        std::vector<Call> calls;
        expr::OpcodeHook hook = [&](const std::string &text, int index, int len,
                                     int64_t &value, std::string &) {
            calls.push_back({text, index, len});
            value = 0x3E;
            return true;
        };
        expr::Result r1 = expr::eval("opcode(\"ld a,n\")", resolver, hook);
        if (!r1.ok || r1.value != 0x3E || calls.size() != 1 ||
            calls[0].text != "ld a,n" || calls[0].index != 0 || calls[0].len != 1) {
            ++g_fail;
            printf("  \033[31mFAIL\033[0m opcode() sans index/len : défauts 0 et 1\n");
        } else ++g_pass;

        calls.clear();
        expr::Result r2 = expr::eval("opcode(\"ld (ix+d),n\",1,2)", resolver, hook);
        if (!r2.ok || calls.size() != 1 || calls[0].text != "ld (ix+d),n" ||
            calls[0].index != 1 || calls[0].len != 2) {
            ++g_fail;
            printf("  \033[31mFAIL\033[0m opcode() transmet index/len tels quels\n");
        } else ++g_pass;
    }
    {
        // Le hook peut échouer (mnémonique invalide, octet non fixe...) : son
        // message remonte tel quel, sans reformulation.
        expr::OpcodeHook fails = [](const std::string &, int, int, int64_t &, std::string &error) {
            error = "byte 1 is not fixed";
            return false;
        };
        expr::Result r = expr::eval("opcode(\"ld a,n\",1)", resolver, fails);
        bool named = !r.ok && r.error.find("byte 1 is not fixed") != std::string::npos;
        if (!named) { ++g_fail; printf("  \033[31mFAIL\033[0m le refus du hook remonte tel quel : %s\n", r.error.c_str()); }
        else ++g_pass;
    }

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
