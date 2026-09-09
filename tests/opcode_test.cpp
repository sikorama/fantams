// opcode_test.cpp - Tests de l'extraction d'octets fixes (ADR 0031)
#include "opcode.h"

#include <cstdio>
#include <string>

static int g_pass = 0, g_fail = 0;

static void chk(const char *desc, const std::string &instr, int index, int len,
                 int64_t expected) {
    opcode::Result r = opcode::extract(instr, index, len);
    if (!r.ok || r.value != expected) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s attendu %lld obtenu %lld%s\n",
               desc, (long long)expected, (long long)r.value,
               r.ok ? "" : (" err: " + r.error).c_str());
    } else ++g_pass;
}

static void chkErrSays(const char *desc, const std::string &instr, int index, int len,
                        const std::string &needle) {
    opcode::Result r = opcode::extract(instr, index, len);
    if (r.ok) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s aurait dû échouer (= %lld)\n", desc, (long long)r.value);
        return;
    }
    if (r.error.find(needle) == std::string::npos) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s le message ne dit pas \"%s\" : %s\n",
               desc, needle.c_str(), r.error.c_str());
    } else ++g_pass;
}

static void chkErr(const char *desc, const std::string &instr, int index, int len) {
    opcode::Result r = opcode::extract(instr, index, len);
    if (r.ok) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-40s aurait dû échouer (= %lld)\n", desc, (long long)r.value);
    } else ++g_pass;
}

int main() {
    // Le premier octet de « ld (bc),a » est 0x02 — cas de référence de l'ADR 0031.
    chk("premier octet de ld a,n", "ld a,n", 0, 1, 0x3E);
    chk("premier octet de ld (bc),a", "ld (bc),a", 0, 1, 0x02);

    // L'octet du placeholder n'est jamais fixe (ADR 0031) : demandé, c'est un refus.
    chkErr("l'octet du placeholder n'est pas disponible", "ld a,n", 1, 1);

    // Une valeur littérale là où un placeholder est attendu : refus explicite (Q17).
    chkErrSays("valeur littérale au lieu d'un placeholder", "ld a,5", 0, 1, "placeholder");

    // Et l'inverse : un placeholder là où une valeur réelle est exigée (le
    // numéro de bit de BIT change l'octet d'opcode lui-même).
    chkErrSays("placeholder au lieu d'une valeur réelle", "bit n,(hl)", 0, 1, "real numeric value");

    // Le numéro de bit réel EST le bon usage, et donne un octet fixe.
    chk("bit 3,(hl) a un premier octet fixe", "bit 3,(hl)", 0, 1, 0xCB);
    chk("bit 3,(hl) : le second octet aussi, il porte le numéro de bit", "bit 3,(hl)", 1, 1, 0x5E);

    // len > 1 : ordre mémoire, byte(index) en poids fort — DD (préfixe IX) puis
    // 7E (LD r,(IX+d) pour A) sont deux octets fixes de « ld a,(ix+d) ».
    chk("len positif : ordre mémoire (poids fort en tête)", "ld a,(ix+d)", 0, 2, 0xDD7E);
    // len < 0 : mêmes octets, mais byte(index) devient le poids faible.
    chk("len négatif : même paire, ordre inversé", "ld a,(ix+d)", 0, -2, 0x7EDD);

    // Le déplacement de jr est un octet relatif au PC : il n'existe pas hors
    // d'un flux d'émission réel, donc jamais fixe — seul le premier octet l'est.
    // (« n », pas « e » : en position nue, le parseur lit d'abord un registre —
    // "e" y désignerait le registre E, pas le placeholder. « d »/« e » ne sont
    // sans ambiguïté que dans "(ix+d)"/"(iy+d)", entre parenthèses.)
    chk("jr c,n : premier octet fixe", "jr c,n", 0, 1, 0x38);
    chkErr("jr c,n : le déplacement ne l'est jamais", "jr c,n", 1, 1);

    // Chaîne qui n'est pas une instruction, et index hors bornes (ADR 0031).
    chkErr("chaîne invalide", "gloubiboulga", 0, 1);
    chkErr("index hors bornes", "nop", 1, 1);
    chkErr("index negatif", "nop", -1, 1);

    printf("\n%d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
