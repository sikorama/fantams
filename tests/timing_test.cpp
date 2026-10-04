// timing_test.cpp - La table des durées Z80, en T-states (ADR 0035)
//
// Les cas s'écrivent comme le source — « ld a,(hl) » — et passent par le
// parseur : un test qui fabriquerait les Operand à la main vérifierait la
// fabrication, pas la lecture d'une ligne. Les valeurs sont celles de la
// documentation Z80 ; elles ne sont déclarées justes qu'après le balayage de
// l'émulateur (ticket 03).
#include "parser.h"
#include "timing.h"

#include <cstdio>
#include <string>

static int g_pass = 0, g_fail = 0;

static timing::Cost costOf(const std::string &line) {
    parser::Result r = parser::parseLine(line);
    if (!r.isInstruction) { timing::Cost c; c.refusal = "not an instruction"; return c; }
    return timing::cost(r.instr);
}

static void chk(const char *line, int tstates, int nops) {
    const timing::Cost c = costOf(line);
    if (!c.ok() || c.tstates != tstates || c.nops != nops) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-22s attendu %d T / %d NOPs, obtenu %d / %d%s%s\n", line,
               tstates, nops, c.tstates, c.nops,
               c.ok() ? "" : " refus : ", c.ok() ? "" : c.refusal);
    } else ++g_pass;
}

static void chkRefused(const char *line, const char *needle) {
    const timing::Cost c = costOf(line);
    if (c.ok()) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-22s aurait dû être refusée (= %d)\n", line, c.tstates);
    } else if (std::string(c.refusal).find(needle) == std::string::npos) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-22s le refus ne dit pas \"%s\" : %s\n", line, needle, c.refusal);
    } else ++g_pass;
}

int main() {
    // Chaque ligne : T-states (Zilog), puis NOPs (Madram / 64NOPS, mesuré sur
    // CPC). Les NOPs ne sont PAS l'arrondi du total : push dure 11 T-states et
    // 4 NOPs, parce que ses phases font 5-3-3 (2+1+1).
    // --- transferts 8 bits
    chk(" ld a,b", 4, 1);           chk(" ld a,7", 7, 2);
    chk(" ld a,(hl)", 7, 2);        chk(" ld (hl),a", 7, 2);      chk(" ld (hl),5", 10, 3);
    chk(" ld a,(ix+3)", 19, 5);     chk(" ld (iy-1),b", 19, 5);   chk(" ld (ix+2),9", 19, 6);
    chk(" ld a,ixh", 8, 2);         chk(" ld ixl,5", 11, 3);
    chk(" ld a,(bc)", 7, 2);        chk(" ld (de),a", 7, 2);
    chk(" ld a,(0x4000)", 13, 4);   chk(" ld (0x4000),a", 13, 4);
    chk(" ld a,i", 9, 3);           chk(" ld r,a", 9, 3);
    // --- transferts 16 bits
    chk(" ld hl,1234", 10, 3);      chk(" ld ix,1234", 14, 4);
    chk(" ld hl,(0x4000)", 16, 5);  chk(" ld (0x4000),hl", 16, 5);
    chk(" ld bc,(0x4000)", 20, 6);  chk(" ld (0x4000),de", 20, 6);
    chk(" ld ix,(0x4000)", 20, 6);  chk(" ld (0x4000),iy", 20, 6);
    chk(" ld sp,hl", 6, 2);         chk(" ld sp,ix", 10, 3);
    chk(" push hl", 11, 4);         chk(" push ix", 15, 5);
    chk(" pop af", 10, 3);          chk(" pop iy", 14, 4);
    chk(" ex de,hl", 4, 1);         chk(" ex af,af'", 4, 1);      chk(" exx", 4, 1);
    chk(" ex (sp),hl", 19, 6);      chk(" ex (sp),ix", 23, 7);
    // --- blocs à durée fixe
    chk(" ldi", 16, 5);             chk(" ldd", 16, 5);            chk(" cpi", 16, 4);
    chk(" cpd", 16, 4);             chk(" ini", 16, 5);            chk(" outd", 16, 5);
    // --- arithmétique et logique
    chk(" add a,b", 4, 1);          chk(" add a,5", 7, 2);         chk(" add a,(hl)", 7, 2);
    chk(" add a,(ix+1)", 19, 5);    chk(" add a,ixh", 8, 2);
    chk(" add b", 4, 1);            chk(" sub 3", 7, 2);           chk(" xor a", 4, 1);
    chk(" cp (hl)", 7, 2);          chk(" and (iy+0)", 19, 5);     chk(" adc a,c", 4, 1);
    chk(" inc b", 4, 1);            chk(" inc (hl)", 11, 3);       chk(" dec (ix+1)", 23, 6);
    chk(" inc ixl", 8, 2);          chk(" inc hl", 6, 2);          chk(" dec sp", 6, 2);
    chk(" inc ix", 10, 3);
    chk(" add hl,de", 11, 3);       chk(" add ix,bc", 15, 4);
    chk(" adc hl,bc", 15, 4);       chk(" sbc hl,de", 15, 4);
    chk(" daa", 4, 1);              chk(" cpl", 4, 1);             chk(" neg", 8, 2);
    chk(" ccf", 4, 1);              chk(" scf", 4, 1);
    // --- rotations, décalages, bits
    chk(" rlca", 4, 1);             chk(" rra", 4, 1);
    chk(" rlc b", 8, 2);            chk(" srl (hl)", 15, 4);       chk(" rl (ix+2)", 23, 7);
    chk(" sla a", 8, 2);
    chk(" rld", 18, 5);             chk(" rrd", 18, 5);
    chk(" bit 3,a", 8, 2);          chk(" bit 3,(hl)", 12, 3);     chk(" bit 0,(ix+1)", 20, 6);
    chk(" res 2,b", 8, 2);          chk(" set 7,(hl)", 15, 4);     chk(" set 1,(iy+4)", 23, 7);
    // --- E/S et CPU : `out (c),r` et `in r,(c)` valent 4 NOPs, pas 3
    chk(" in a,(0xFE)", 11, 3);     chk(" out (0xFE),a", 11, 3);
    chk(" in b,(c)", 12, 4);        chk(" out (c),d", 12, 4);      chk(" out (c),a", 12, 4);
    chk(" out (c),0", 12, 4);
    chk(" nop", 4, 1);              chk(" di", 4, 1);              chk(" ei", 4, 1);
    chk(" im 1", 8, 2);

    // --- ce qui n'a PAS de durée fixe : refusé, avec sa raison
    chkRefused(" jp 0x4000", "transfer control");
    chkRefused(" jp (hl)", "transfer control");
    chkRefused(" jr nz,0x4000", "transfer control");
    chkRefused(" djnz 0x4000", "transfer control");
    chkRefused(" call 0x4000", "transfer control");
    chkRefused(" ret", "transfer control");
    chkRefused(" ret z", "transfer control");
    chkRefused(" reti", "transfer control");
    chkRefused(" rst 0x38", "transfer control");
    chkRefused(" ldir", "counter");
    chkRefused(" lddr", "counter");
    chkRefused(" cpir", "counter");
    chkRefused(" otir", "counter");
    chkRefused(" halt", "interrupt");

    printf("%d passes, %d echecs\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
