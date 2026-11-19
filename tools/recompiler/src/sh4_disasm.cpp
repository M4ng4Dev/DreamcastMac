#include "sh4_disasm.h"
#include <sstream>
#include <iomanip>
#include <queue>

namespace dc::recomp {

DecodedInstruction SH4Disassembler::decode(uint32_t pc, uint16_t op) {
    DecodedInstruction inst{};
    inst.address = pc;
    inst.raw_opcode = op;
    inst.type = OpcodeType::UNKNOWN;
    inst.rn = (op >> 8) & 0xF;
    inst.rm = (op >> 4) & 0xF;
    inst.imm = 0;
    inst.is_branch = false;
    inst.has_delay_slot = false;

    // Helper lambdas for sign extension
    auto sign_ext8 = [](uint8_t val) -> int32_t {
        return static_cast<int8_t>(val);
    };
    auto sign_ext12 = [](uint16_t val) -> int32_t {
        if (val & 0x800) return static_cast<int32_t>(val | 0xFFFFF000);
        return static_cast<int32_t>(val);
    };

    // 0000 0000 0000 1001: NOP
    if (op == 0x0009) {
        inst.type = OpcodeType::NOP;
        inst.mnemonic = "nop";
        return inst;
    }

    // 0000 0000 0000 1011: RTS (delayed)
    if (op == 0x000B) {
        inst.type = OpcodeType::RTS;
        inst.is_branch = true;
        inst.has_delay_slot = true;
        inst.mnemonic = "rts";
        return inst;
    }

    uint8_t op_hi = (op >> 12) & 0xF;
    uint8_t op_lo = op & 0xF;

    switch (op_hi) {
        case 0x1: // mov.l Rm, @(disp, Rn)
            inst.type = OpcodeType::MOV_STORE;
            inst.imm = (op & 0xF) * 4;
            inst.mnemonic = "mov.l r" + std::to_string(inst.rm) + ", @(" + std::to_string(inst.imm) + ", r" + std::to_string(inst.rn) + ")";
            return inst;

        case 0x2: // mov.l Rm, @Rn
            if (op_lo == 0x2) {
                inst.type = OpcodeType::MOV_STORE;
                inst.mnemonic = "mov.l r" + std::to_string(inst.rm) + ", @r" + std::to_string(inst.rn);
                return inst;
            }
            break;

        case 0x3: // ALU Rm, Rn
            switch (op_lo) {
                case 0x0:
                    inst.type = OpcodeType::CMP_EQ;
                    inst.mnemonic = "cmp/eq r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0x2:
                    inst.type = OpcodeType::CMP_HS;
                    inst.mnemonic = "cmp/hs r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0x3:
                    inst.type = OpcodeType::CMP_GE;
                    inst.mnemonic = "cmp/ge r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0x6:
                    inst.type = OpcodeType::CMP_HI;
                    inst.mnemonic = "cmp/hi r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0x7:
                    inst.type = OpcodeType::CMP_GT;
                    inst.mnemonic = "cmp/gt r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0x8:
                    inst.type = OpcodeType::SUB;
                    inst.mnemonic = "sub r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0xA:
                    inst.type = OpcodeType::SUBC;
                    inst.mnemonic = "subc r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
                case 0xC:
                    inst.type = OpcodeType::ADD;
                    inst.mnemonic = "add r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                    return inst;
            }
            break;

        case 0x4: // Branch indirect or special ALU
            if ((op & 0xFF) == 0x2B) { // jmp @Rn
                inst.type = OpcodeType::JMP;
                inst.is_branch = true;
                inst.has_delay_slot = true;
                inst.mnemonic = "jmp @r" + std::to_string(inst.rn);
                return inst;
            }
            if ((op & 0xFF) == 0x0B) { // jsr @Rn
                inst.type = OpcodeType::JSR;
                inst.is_branch = true;
                inst.has_delay_slot = true;
                inst.mnemonic = "jsr @r" + std::to_string(inst.rn);
                return inst;
            }
            break;

        case 0x5: // mov.l @(disp, Rm), Rn
            inst.type = OpcodeType::MOV_LOAD;
            inst.imm = (op & 0xF) * 4;
            inst.mnemonic = "mov.l @(" + std::to_string(inst.imm) + ", r" + std::to_string(inst.rm) + "), r" + std::to_string(inst.rn);
            return inst;

        case 0x6: // mov Rm, Rn or mov.l @Rm, Rn
            if (op_lo == 0x2) {
                inst.type = OpcodeType::MOV_LOAD;
                inst.mnemonic = "mov.l @r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                return inst;
            }
            if (op_lo == 0x3) {
                inst.type = OpcodeType::MOV;
                inst.mnemonic = "mov r" + std::to_string(inst.rm) + ", r" + std::to_string(inst.rn);
                return inst;
            }
            break;

        case 0x7: // add #imm, Rn
            inst.type = OpcodeType::ADD_IMM;
            inst.imm = sign_ext8(op & 0xFF);
            inst.mnemonic = "add #" + std::to_string(inst.imm) + ", r" + std::to_string(inst.rn);
            return inst;

        case 0x8: // Branch conditional
            switch ((op >> 8) & 0xF) {
                case 0x9: // bt disp
                    inst.type = OpcodeType::BT;
                    inst.is_branch = true;
                    inst.target_pc = pc + 4 + (sign_ext8(op & 0xFF) * 2);
                    {
                        std::ostringstream ss;
                        ss << "bt 0x" << std::hex << inst.target_pc;
                        inst.mnemonic = ss.str();
                    }
                    return inst;
                case 0xB: // bf disp
                    inst.type = OpcodeType::BF;
                    inst.is_branch = true;
                    inst.target_pc = pc + 4 + (sign_ext8(op & 0xFF) * 2);
                    {
                        std::ostringstream ss;
                        ss << "bf 0x" << std::hex << inst.target_pc;
                        inst.mnemonic = ss.str();
                    }
                    return inst;
                case 0xD: // bt/s disp
                    inst.type = OpcodeType::BT_S;
                    inst.is_branch = true;
                    inst.has_delay_slot = true;
                    inst.target_pc = pc + 4 + (sign_ext8(op & 0xFF) * 2);
                    {
                        std::ostringstream ss;
                        ss << "bt/s 0x" << std::hex << inst.target_pc;
                        inst.mnemonic = ss.str();
                    }
                    return inst;
                case 0xF: // bf/s disp
                    inst.type = OpcodeType::BF_S;
                    inst.is_branch = true;
                    inst.has_delay_slot = true;
                    inst.target_pc = pc + 4 + (sign_ext8(op & 0xFF) * 2);
                    {
                        std::ostringstream ss;
                        ss << "bf/s 0x" << std::hex << inst.target_pc;
                        inst.mnemonic = ss.str();
                    }
                    return inst;
            }
            break;

        case 0xA: // bra disp (delayed)
            inst.type = OpcodeType::BRA;
            inst.is_branch = true;
            inst.has_delay_slot = true;
            inst.target_pc = pc + 4 + (sign_ext12(op & 0xFFF) * 2);
            {
                std::ostringstream ss;
                ss << "bra 0x" << std::hex << inst.target_pc;
                inst.mnemonic = ss.str();
            }
            return inst;

        case 0xB: // bsr disp (delayed)
            inst.type = OpcodeType::BSR;
            inst.is_branch = true;
            inst.has_delay_slot = true;
            inst.target_pc = pc + 4 + (sign_ext12(op & 0xFFF) * 2);
            {
                std::ostringstream ss;
                ss << "bsr 0x" << std::hex << inst.target_pc;
                inst.mnemonic = ss.str();
            }
            return inst;

        case 0xE: // mov #imm, Rn
            inst.type = OpcodeType::MOV_IMM;
            inst.imm = sign_ext8(op & 0xFF);
            inst.mnemonic = "mov #" + std::to_string(inst.imm) + ", r" + std::to_string(inst.rn);
            return inst;

        case 0xF: // Floating-Point and Vector Instructions
            if ((op & 0xF) == 0xE) { // fipr FVm, FVn
                inst.type = OpcodeType::FIPR;
                inst.rn = (op >> 8) & 0xC; // Vector index 0, 4, 8, 12
                inst.rm = (op >> 4) & 0xC;
                inst.mnemonic = "fipr fv" + std::to_string(inst.rm) + ", fv" + std::to_string(inst.rn);
                return inst;
            }
            if (op == 0xF1FD || op == 0xF5FD || op == 0xF9FD || op == 0xFDFD) { // ftrv xmtrx, FVn
                inst.type = OpcodeType::FTRV;
                inst.rn = (op >> 8) & 0xC;
                inst.mnemonic = "ftrv xmtrx, fv" + std::to_string(inst.rn);
                return inst;
            }
            switch (op_lo) {
                case 0x0:
                    inst.type = OpcodeType::FADD;
                    inst.mnemonic = "fadd fr" + std::to_string(inst.rm) + ", fr" + std::to_string(inst.rn);
                    return inst;
                case 0x1:
                    inst.type = OpcodeType::FSUB;
                    inst.mnemonic = "fsub fr" + std::to_string(inst.rm) + ", fr" + std::to_string(inst.rn);
                    return inst;
                case 0x2:
                    inst.type = OpcodeType::FMUL;
                    inst.mnemonic = "fmul fr" + std::to_string(inst.rm) + ", fr" + std::to_string(inst.rn);
                    return inst;
                case 0x3:
                    inst.type = OpcodeType::FDIV;
                    inst.mnemonic = "fdiv fr" + std::to_string(inst.rm) + ", fr" + std::to_string(inst.rn);
                    return inst;
                case 0xC:
                    inst.type = OpcodeType::FMOV;
                    inst.mnemonic = "fmov fr" + std::to_string(inst.rm) + ", fr" + std::to_string(inst.rn);
                    return inst;
            }
            break;
    }

    std::ostringstream ss;
    ss << "unknown 0x" << std::hex << std::setfill('0') << std::setw(4) << op;
    inst.mnemonic = ss.str();
    return inst;
}

std::vector<FunctionBlock> SH4Disassembler::analyze(
    const uint8_t* code_bytes,
    size_t code_size,
    uint32_t base_address,
    const std::set<uint32_t>& entry_points
) {
    std::vector<FunctionBlock> functions;
    std::set<uint32_t> visited_blocks;
    std::queue<uint32_t> pending_functions;

    for (uint32_t ep : entry_points) {
        pending_functions.push(ep);
    }

    while (!pending_functions.empty()) {
        uint32_t func_pc = pending_functions.front();
        pending_functions.pop();

        if (visited_blocks.count(func_pc) > 0) continue;

        FunctionBlock func;
        func.entry_pc = func_pc;
        std::ostringstream ss;
        ss << "func_0x" << std::hex << func_pc;
        func.name = ss.str();

        BasicBlock current_bb;
        current_bb.start_pc = func_pc;
        uint32_t curr_pc = func_pc;

        while (true) {
            if (curr_pc < base_address || (curr_pc - base_address + 2) > code_size) {
                break;
            }

            uint32_t offset = curr_pc - base_address;
            uint16_t op = static_cast<uint16_t>(code_bytes[offset] | (code_bytes[offset + 1] << 8));
            DecodedInstruction inst = decode(curr_pc, op);
            current_bb.instructions.push_back(inst);
            visited_blocks.insert(curr_pc);
            curr_pc += 2;

            if (inst.is_branch) {
                if (inst.has_delay_slot && (curr_pc - base_address + 2) <= code_size) {
                    uint32_t delay_offset = curr_pc - base_address;
                    uint16_t delay_op = static_cast<uint16_t>(code_bytes[delay_offset] | (code_bytes[delay_offset + 1] << 8));
                    DecodedInstruction delay_inst = decode(curr_pc, delay_op);
                    current_bb.instructions.push_back(delay_inst);
                    visited_blocks.insert(curr_pc);
                    curr_pc += 2;
                }

                current_bb.end_pc = curr_pc;
                func.blocks.push_back(current_bb);

                // If BSR / JSR branch, push destination function to discover
                if (inst.type == OpcodeType::BSR && inst.target_pc != 0) {
                    pending_functions.push(inst.target_pc);
                }
                break;
            }
        }

        if (!func.blocks.empty()) {
            functions.push_back(func);
        }
    }

    return functions;
}

} // namespace dc::recomp
