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

static void chk(const char *line, int tstates) {
    const timing::Cost c = costOf(line);
    if (!c.ok() || c.tstates != tstates) {
        ++g_fail;
        printf("  \033[31mFAIL\033[0m %-22s attendu %d obtenu %d%s%s\n", line, tstates, c.tstates,
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

static void chkInt(const char *what, int got, int want) {
    if (got != want) { ++g_fail; printf("  \033[31mFAIL\033[0m %-22s attendu %d obtenu %d\n", what, want, got); }
    else ++g_pass;
}

int main() {
    // --- transferts 8 bits
    chk(" ld a,b", 4);            chk(" ld a,7", 7);
    chk(" ld a,(hl)", 7);         chk(" ld (hl),a", 7);       chk(" ld (hl),5", 10);
    chk(" ld a,(ix+3)", 19);      chk(" ld (iy-1),b", 19);    chk(" ld (ix+2),9", 19);
    chk(" ld a,ixh", 8);          chk(" ld ixl,5", 11);
    chk(" ld a,(bc)", 7);         chk(" ld (de),a", 7);
    chk(" ld a,(0x4000)", 13);    chk(" ld (0x4000),a", 13);
    chk(" ld a,i", 9);            chk(" ld r,a", 9);
    // --- transferts 16 bits
    chk(" ld hl,1234", 10);       chk(" ld ix,1234", 14);
    chk(" ld hl,(0x4000)", 16);   chk(" ld (0x4000),hl", 16);
    chk(" ld bc,(0x4000)", 20);   chk(" ld (0x4000),de", 20);
    chk(" ld ix,(0x4000)", 20);   chk(" ld (0x4000),iy", 20);
    chk(" ld sp,hl", 6);          chk(" ld sp,ix", 10);
    chk(" push hl", 11);          chk(" push ix", 15);
    chk(" pop af", 10);           chk(" pop iy", 14);
    chk(" ex de,hl", 4);          chk(" ex af,af'", 4);       chk(" exx", 4);
    chk(" ex (sp),hl", 19);       chk(" ex (sp),ix", 23);
    // --- blocs à durée fixe
    chk(" ldi", 16);              chk(" ldd", 16);            chk(" cpi", 16);
    chk(" ini", 16);              chk(" outd", 16);
    // --- arithmétique et logique
    chk(" add a,b", 4);           chk(" add a,5", 7);         chk(" add a,(hl)", 7);
    chk(" add a,(ix+1)", 19);     chk(" add a,ixh", 8);
    chk(" add b", 4);             chk(" sub 3", 7);           chk(" xor a", 4);
    chk(" cp (hl)", 7);           chk(" and (iy+0)", 19);     chk(" adc a,c", 4);
    chk(" inc b", 4);             chk(" inc (hl)", 11);       chk(" dec (ix+1)", 23);
    chk(" inc ixl", 8);           chk(" inc hl", 6);          chk(" dec sp", 6);
    chk(" inc ix", 10);
    chk(" add hl,de", 11);        chk(" add ix,bc", 15);
    chk(" adc hl,bc", 15);        chk(" sbc hl,de", 15);
    chk(" daa", 4);               chk(" cpl", 4);             chk(" neg", 8);
    chk(" ccf", 4);               chk(" scf", 4);
    // --- rotations, décalages, bits
    chk(" rlca", 4);              chk(" rra", 4);
    chk(" rlc b", 8);             chk(" srl (hl)", 15);       chk(" rl (ix+2)", 23);
    chk(" sla a", 8);
    chk(" rld", 18);              chk(" rrd", 18);
    chk(" bit 3,a", 8);           chk(" bit 3,(hl)", 12);     chk(" bit 0,(ix+1)", 20);
    chk(" res 2,b", 8);           chk(" set 7,(hl)", 15);     chk(" set 1,(iy+4)", 23);
    // --- E/S et CPU
    chk(" in a,(0xFE)", 11);      chk(" out (0xFE),a", 11);
    chk(" in b,(c)", 12);         chk(" out (c),d", 12);      chk(" out (c),0", 12);
    chk(" nop", 4);               chk(" di", 4);              chk(" ei", 4);
    chk(" im 1", 8);

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

    // --- l'arrondi au NOP, instruction par instruction
    chkInt("nops(4)", timing::nops(4, 4), 1);
    chkInt("nops(7)", timing::nops(7, 4), 2);
    chkInt("nops(11)", timing::nops(11, 4), 3);
    chkInt("nops(13)", timing::nops(13, 4), 4);
    chkInt("nops(19)", timing::nops(19, 4), 5);
    chkInt("nops(23)", timing::nops(23, 4), 6);

    printf("%d passes, %d echecs\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
