// z80.h - Encodeur Z80 data-driven pour fantams
//
// Ce module est autonome : il ne dépend d'AUCUN autre module du projet.
// Il encode une instruction Z80 (mnémonique + opérandes) en octets, via une
// interface abstraite IAsmContext que l'hôte fournit (émission, évaluation
// d'expression, PC courant, erreurs). Les expressions ne sont PAS évaluées
// ici : c'est le rôle du contexte (ce qui autorise la résolution différée /
// forward references côté assembleur).
//
// Portage WASM/JS : aucune allocation manuelle, aucune I/O, aucun état global.
#pragma once

#include <cstdint>
#include <string>

namespace z80 {

// --- Registres --------------------------------------------------------------
enum class Reg {
    None,
    // 8 bits documentés
    A, B, C, D, E, H, L,
    // 8 bits non documentés (moitiés de IX/IY)
    IXH, IXL, IYH, IYL,
    // registres spéciaux
    I, R,
    // 16 bits
    AF, BC, DE, HL, SP, IX, IY,
    AFp, // AF'
};

// --- Codes conditions -------------------------------------------------------
enum class Cond { None, NZ, Z, NC, C, PO, PE, P, M };

// --- Mnémoniques ------------------------------------------------------------
enum class Mnemo {
    Invalid,
    // transfert
    LD, PUSH, POP, EX, EXX, LDI, LDIR, LDD, LDDR,
    // arithmétique / logique 8/16 bits
    ADD, ADC, SUB, SBC, AND, XOR, OR, CP, INC, DEC,
    DAA, CPL, NEG, CCF, SCF,
    CPI, CPIR, CPD, CPDR,
    // rotations / décalages
    RLCA, RRCA, RLA, RRA, RLC, RRC, RL, RR, SLA, SRA, SLL, SRL, RLD, RRD,
    // bits
    BIT, RES, SET,
    // saut / appel / retour
    JP, JR, DJNZ, CALL, RET, RETI, RETN, RST,
    // CPU / interruptions
    NOP, HALT, DI, EI, IM,
    // E/S
    IN, OUT, INI, INIR, IND, INDR, OUTI, OTIR, OUTD, OTDR,
};

// --- Opérande ---------------------------------------------------------------
struct Operand {
    enum class Kind {
        None,
        Reg,     // un registre (champ reg)
        RegInd,  // (BC) (DE) (HL) (SP) (C)  -> reg = BC/DE/HL/SP/C
        Indexed, // (IX+d) / (IY+d)          -> reg = IX/IY, expr = déplacement
        Imm,     // valeur immédiate n / nn / numéro de bit / vecteur RST -> expr
        MemImm,  // (nn) adressage absolu mémoire                          -> expr
        Cond,    // code condition                                          -> cc
    };
    Kind kind = Kind::None;
    Reg reg = Reg::None;
    Cond cc = Cond::None;
    std::string expr; // pour Imm / MemImm / déplacement Indexed

    // fabriques pratiques (le futur parseur les utilisera)
    static Operand none() { return {}; }
    static Operand r(Reg rr) { Operand o; o.kind = Kind::Reg; o.reg = rr; return o; }
    static Operand rind(Reg rr) { Operand o; o.kind = Kind::RegInd; o.reg = rr; return o; }
    static Operand idx(Reg xy, std::string disp) { Operand o; o.kind = Kind::Indexed; o.reg = xy; o.expr = std::move(disp); return o; }
    static Operand imm(std::string e) { Operand o; o.kind = Kind::Imm; o.expr = std::move(e); return o; }
    static Operand mem(std::string e) { Operand o; o.kind = Kind::MemImm; o.expr = std::move(e); return o; }
    static Operand condition(Cond c) { Operand o; o.kind = Kind::Cond; o.cc = c; return o; }
};

struct Instruction {
    Mnemo mnemo = Mnemo::Invalid;
    Operand a; // 1er opérande (ou None)
    Operand b; // 2e opérande (ou None)
};

// Ce que l'encodeur peut demander au linker d'écrire à sa place, quand la cible
// n'a pas encore d'adresse. Il n'en connaît que la FORME de ce qu'il émet :
// deux octets d'adresse, un déplacement relatif, ou un seul octet — lequel des
// deux, `high()` ou `low()`, est l'affaire de l'expression, donc du contexte.
enum class RelocKind { Abs16, Rel8, Byte };

// --- Interface de contexte (frontière avec le reste de l'assembleur) --------
// L'encodeur n'appelle QUE ces méthodes. Un contexte factice suffit à le tester.
struct IAsmContext {
    virtual ~IAsmContext() = default;
    // Émet un octet dans le flux de sortie et avance le PC de sortie.
    virtual void emit(uint8_t b) = 0;
    // Évalue une expression et renvoie sa valeur. Peut être différée côté hôte ;
    // ici on suppose une valeur disponible (l'hôte gère la 2e passe).
    //
    // REFUSE une cible dont l'adresse n'est pas encore connue : les contextes qui
    // en acceptent une passent par `evalAddr` ou `rel8`, et tout le reste — un
    // numéro de bit, un vecteur RST, un déplacement indexé — n'en a jamais.
    virtual int64_t eval(const std::string &expr) = 0;
    // Adresse courante (PC logique) AVANT émission de l'instruction courante.
    // Nécessaire pour l'adressage relatif (JR / DJNZ).
    virtual uint16_t pc() const = 0;
    // Signale une erreur d'encodage (combinaison mnémonique/opérandes invalide,
    // déplacement hors bornes, etc.).
    virtual void error(const std::string &msg) = 0;

    // --- Cibles dont l'adresse peut n'être connue qu'au linkage --------------
    // Le §10 rangeait l'encodeur parmi les fichiers que la relocalisation ne
    // concerne pas. C'est faux : il calcule LUI-MÊME le déplacement d'un `jr` et
    // refuse ce qui sort de [-128, 127]. Il ne peut plus le faire sur une cible
    // qu'il ne connaît pas — et intercepter en amont rendrait muet le
    // diagnostic le plus utile du Z80. Le fait « cette cible n'est pas connue »
    // appartient donc à l'endroit qui l'encode.

    // Évalue une expression qui peut désigner une adresse pas encore décidée.
    // Rend la partie CONNUE, et met `relocatable` à true si une base de section
    // s'y ajoute — auquel cas l'appelant demande une relocalisation.
    virtual int64_t evalAddr(const std::string &expr, bool &relocatable) {
        relocatable = false;
        return eval(expr);
    }
    // Le déplacement d'un saut relatif, `pcNext` étant l'adresse de
    // l'instruction SUIVANTE. Rend false quand la distance n'est pas connue
    // ici : l'encodeur émet alors un octet de garde et demande une `Rel8`.
    virtual bool rel8(const std::string &expr, int64_t pcNext, int64_t &disp) {
        disp = eval(expr) - pcNext;
        return true;
    }
    // Demande une relocalisation portant sur les octets qui commencent au
    // PROCHAIN `emit`.
    virtual void reloc(const std::string &expr, RelocKind kind) { (void)expr; (void)kind; }
};

// Encode une instruction. Renvoie true si encodée, false si combinaison
// invalide (dans ce cas ctx.error() a été appelé et rien n'est émis).
bool encode(IAsmContext &ctx, const Instruction &in);

// Utilitaires exposés (pratiques pour le futur parseur et les tests).
Mnemo mnemoFromString(const std::string &s); // "LD" -> Mnemo::LD, sinon Invalid
const char *mnemoName(Mnemo m);

} // namespace z80
