#pragma once
#include "sh4_opcodes.h"
#include <vector>
#include <map>
#include <set>

namespace dc::recomp {

struct BasicBlock {
    uint32_t start_pc;
    uint32_t end_pc;
    std::vector<DecodedInstruction> instructions;
    std::vector<uint32_t> successors;
};

struct FunctionBlock {
    uint32_t entry_pc;
    std::string name;
    std::vector<BasicBlock> blocks;
};

class SH4Disassembler {
public:
    static DecodedInstruction decode(uint32_t pc, uint16_t op);

    // Full binary analysis and function boundary discovery
    static std::vector<FunctionBlock> analyze(
        const uint8_t* code_bytes,
        size_t code_size,
        uint32_t base_address,
        const std::set<uint32_t>& entry_points
    );
};

} // namespace dc::recomp
