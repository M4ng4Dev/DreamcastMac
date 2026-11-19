#pragma once
#include <cstdint>
#include <string>

namespace dc::recomp {

enum class OpcodeType {
    UNKNOWN,
    // Integer Arithmetic & Logic
    MOV,
    MOV_IMM,
    MOV_LOAD,
    MOV_STORE,
    ADD,
    ADD_IMM,
    ADDC,
    SUB,
    SUBC,
    CMP_EQ,
    CMP_GE,
    CMP_GT,
    CMP_HI,
    CMP_HS,
    AND,
    OR,
    XOR,
    NOT,
    SHLL,
    SHLR,
    SHAR,
    ROTL,
    ROTR,
    MULU,
    MULS,
    DMULU,
    DMULS,
    // Control Flow / Branches
    BF,
    BT,
    BF_S,   // Delayed
    BT_S,   // Delayed
    BRA,    // Delayed
    BRAF,   // Delayed
    BSR,    // Delayed
    BSRF,   // Delayed
    JMP,    // Delayed
    JSR,    // Delayed
    RTS,    // Delayed
    NOP,
    // Floating-Point & Vector
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FSQRT,
    FIPR,   // 4D Vector Inner Product (NEON accelerated)
    FTRV,   // 4D Matrix Vector Transform (NEON accelerated)
    FMOV,
    FLDI0,
    FLDI1,
    FSTS,
    FLDS,
    FCMP_EQ,
    FCMP_GT
};

struct DecodedInstruction {
    uint32_t address;
    uint16_t raw_opcode;
    OpcodeType type;
    int rn;             // Destination / Primary Register
    int rm;             // Source / Secondary Register
    int32_t imm;        // Immediate or displacement
    uint32_t target_pc; // Target branch address (if known)
    bool is_branch;
    bool has_delay_slot;
    std::string mnemonic;
};

} // namespace dc::recomp
