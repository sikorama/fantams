// timing.cpp - Durée d'une instruction Z80, en T-states (voir timing.h)
//
// Les valeurs sont celles de la documentation Z80 (Zilog UM0080), pour les
// seules instructions à durée FIXE. Elles ne sont déclarées justes qu'après
// confrontation à l'émulateur (ADR 0035, ticket 03) : jusque-là, c'est une table
// documentaire, et le balayage est ce qui la tient.
#include "timing.h"

namespace timing {
namespace {

using z80::Mnemo;
using z80::Operand;
using z80::Reg;
using Kind = Operand::Kind;

const char *kBranch = "it can transfer control: how long it takes depends on the path taken";
const char *kRepeat = "it repeats: how long it takes depends on a counter";
const char *kHalt   = "it waits for an interrupt";
const char *kUnknown = "its duration is not known for this form";

// La nature d'un opérande 8 bits, vue du seul chronomètre.
enum class R8 { None, Reg, Half, HL, Indexed };
R8 r8(const Operand &o) {
    if (o.kind == Kind::Reg) {
        switch (o.reg) {
            case Reg::A: case Reg::B: case Reg::C: case Reg::D:
            case Reg::E: case Reg::H: case Reg::L: return R8::Reg;
            case Reg::IXH: case Reg::IXL: case Reg::IYH: case Reg::IYL: return R8::Half;
            default: return R8::None;
        }
    }
    if (o.kind == Kind::RegInd && o.reg == Reg::HL) return R8::HL;
    if (o.kind == Kind::Indexed) return R8::Indexed;
    if (o.kind == Kind::RegInd && (o.reg == Reg::IX || o.reg == Reg::IY)) return R8::Indexed;
    return R8::None;
}

bool isIdx(Reg r) { return r == Reg::IX || r == Reg::IY; }
bool is(const Operand &o, Reg r) { return o.kind == Kind::Reg && o.reg == r; }
bool isInd(const Operand &o, Reg r) { return o.kind == Kind::RegInd && o.reg == r; }

Cost fixed(int t) { Cost c; c.tstates = t; return c; }
Cost refused(const char *why) { Cost c; c.refusal = why; return c; }

// ALU 8 bits, ou le travail d'un opérande source : r 4, n 7, (HL) 7, (IX+d) 19.
Cost alu(const Operand &src) {
    if (src.kind == Kind::Imm) return fixed(7);
    switch (r8(src)) {
        case R8::Reg: return fixed(4);
        case R8::Half: return fixed(8);
        case R8::HL: return fixed(7);
        case R8::Indexed: return fixed(19);
        default: return refused(kUnknown);
    }
}

Cost ld(const Operand &a, const Operand &b) {
    const R8 ra = r8(a), rb = r8(b);
    if (ra != R8::None && rb != R8::None) {
        if (ra == R8::Indexed || rb == R8::Indexed) return fixed(19);
        if (ra == R8::HL || rb == R8::HL) return fixed(7);
        if (ra == R8::Half || rb == R8::Half) return fixed(8);
        return fixed(4);
    }
    if (ra != R8::None && b.kind == Kind::Imm) {
        switch (ra) {
            case R8::Indexed: return fixed(19);
            case R8::HL: return fixed(10);
            case R8::Half: return fixed(11);
            default: return fixed(7);
        }
    }
    if (is(a, Reg::A) && (isInd(b, Reg::BC) || isInd(b, Reg::DE))) return fixed(7);
    if ((isInd(a, Reg::BC) || isInd(a, Reg::DE)) && is(b, Reg::A)) return fixed(7);
    if ((is(a, Reg::A) && b.kind == Kind::MemImm) ||
        (a.kind == Kind::MemImm && is(b, Reg::A))) return fixed(13);
    if ((is(a, Reg::A) && (is(b, Reg::I) || is(b, Reg::R))) ||
        ((is(a, Reg::I) || is(a, Reg::R)) && is(b, Reg::A))) return fixed(9);
    if (is(a, Reg::SP)) {
        if (is(b, Reg::HL)) return fixed(6);
        if (b.kind == Kind::Reg && isIdx(b.reg)) return fixed(10);
    }
    if (a.kind == Kind::Reg && b.kind == Kind::Imm) return fixed(isIdx(a.reg) ? 14 : 10);
    if (a.kind == Kind::Reg && b.kind == Kind::MemImm)
        return fixed(a.reg == Reg::HL ? 16 : 20);
    if (a.kind == Kind::MemImm && b.kind == Kind::Reg)
        return fixed(b.reg == Reg::HL ? 16 : 20);
    return refused(kUnknown);
}

} // namespace

Cost cost(const z80::Instruction &in) {
    const Operand &a = in.a, &b = in.b;
    switch (in.mnemo) {
        case Mnemo::LD: return ld(a, b);

        case Mnemo::PUSH: return fixed(a.kind == Kind::Reg && isIdx(a.reg) ? 15 : 11);
        case Mnemo::POP:  return fixed(a.kind == Kind::Reg && isIdx(a.reg) ? 14 : 10);

        case Mnemo::EX:
            if (isInd(a, Reg::SP)) return fixed(b.kind == Kind::Reg && isIdx(b.reg) ? 23 : 19);
            return fixed(4);                     // ex de,hl · ex af,af'
        case Mnemo::EXX: return fixed(4);

        case Mnemo::ADD: case Mnemo::ADC: case Mnemo::SBC:
            if (a.kind == Kind::Reg && (a.reg == Reg::HL || isIdx(a.reg))) {
                if (in.mnemo == Mnemo::ADD) return fixed(a.reg == Reg::HL ? 11 : 15);
                return fixed(15);                // adc/sbc hl,rr
            }
            return alu(is(a, Reg::A) && b.kind != Kind::None ? b : a);
        case Mnemo::SUB: case Mnemo::AND: case Mnemo::XOR: case Mnemo::OR: case Mnemo::CP:
            return alu(is(a, Reg::A) && b.kind != Kind::None ? b : a);

        case Mnemo::INC: case Mnemo::DEC:
            if (a.kind == Kind::Reg && (a.reg == Reg::BC || a.reg == Reg::DE ||
                                        a.reg == Reg::HL || a.reg == Reg::SP))
                return fixed(6);
            if (a.kind == Kind::Reg && isIdx(a.reg)) return fixed(10);
            switch (r8(a)) {
                case R8::Reg: return fixed(4);
                case R8::Half: return fixed(8);
                case R8::HL: return fixed(11);
                case R8::Indexed: return fixed(23);
                default: return refused(kUnknown);
            }

        case Mnemo::DAA: case Mnemo::CPL: case Mnemo::CCF: case Mnemo::SCF:
        case Mnemo::RLCA: case Mnemo::RRCA: case Mnemo::RLA: case Mnemo::RRA:
        case Mnemo::NOP: case Mnemo::DI: case Mnemo::EI:
            return fixed(4);
        case Mnemo::NEG: case Mnemo::IM: return fixed(8);
        case Mnemo::RLD: case Mnemo::RRD: return fixed(18);

        case Mnemo::RLC: case Mnemo::RRC: case Mnemo::RL: case Mnemo::RR:
        case Mnemo::SLA: case Mnemo::SRA: case Mnemo::SLL: case Mnemo::SRL:
            switch (r8(a)) {
                case R8::Reg: return fixed(8);
                case R8::HL: return fixed(15);
                case R8::Indexed: return fixed(23);
                default: return refused(kUnknown);
            }
        // BIT lit, RES et SET écrivent : (HL) 12 contre 15, (IX+d) 20 contre 23.
        case Mnemo::BIT:
            switch (r8(b)) {
                case R8::Reg: return fixed(8);
                case R8::HL: return fixed(12);
                case R8::Indexed: return fixed(20);
                default: return refused(kUnknown);
            }
        case Mnemo::RES: case Mnemo::SET:
            switch (r8(b)) {
                case R8::Reg: return fixed(8);
                case R8::HL: return fixed(15);
                case R8::Indexed: return fixed(23);
                default: return refused(kUnknown);
            }

        case Mnemo::IN: case Mnemo::OUT:
            // `in a,(n)` et `out (n),a` : 11 ; la forme `(c)` : 12.
            return fixed((a.kind == Kind::MemImm || b.kind == Kind::MemImm) ? 11 : 12);

        case Mnemo::LDI: case Mnemo::LDD: case Mnemo::CPI: case Mnemo::CPD:
        case Mnemo::INI: case Mnemo::IND: case Mnemo::OUTI: case Mnemo::OUTD:
            return fixed(16);

        case Mnemo::LDIR: case Mnemo::LDDR: case Mnemo::CPIR: case Mnemo::CPDR:
        case Mnemo::INIR: case Mnemo::INDR: case Mnemo::OTIR: case Mnemo::OTDR:
            return refused(kRepeat);

        case Mnemo::JP: case Mnemo::JR: case Mnemo::DJNZ: case Mnemo::CALL:
        case Mnemo::RET: case Mnemo::RETI: case Mnemo::RETN: case Mnemo::RST:
            return refused(kBranch);

        case Mnemo::HALT: return refused(kHalt);

        default: return refused(kUnknown);
    }
}

int nops(int tstates, int tstatesPerNop) {
    return (tstates + tstatesPerNop - 1) / tstatesPerNop;
}

} // namespace timing
