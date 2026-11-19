#include "sh4_context.h"
#include <iostream>
#include <cassert>

// Forward declaration of the generated dispatcher
extern void dispatch_sh4(dc::runtime::SH4Context& ctx, uint32_t target_pc);

int main() {
    std::cout << "========================================================\n"
              << " DreamcastMac - Native Execution Verification Test\n"
              << "========================================================\n";

    dc::runtime::SH4Context ctx{};
    ctx.reset(0x8C010000);

    std::cout << "[Initial State] PC = 0x" << std::hex << ctx.pc
              << ", R0 = " << std::dec << ctx.r[0]
              << ", R1 = " << ctx.r[1] << "\n";

    std::cout << "Executing recompiled SH-4 code natively...\n";
    dispatch_sh4(ctx, 0x8C010000);

    std::cout << "[Final State]   R0 = " << ctx.r[0]
              << ", R1 = " << ctx.r[1]
              << ", PR = 0x" << std::hex << ctx.pr << std::dec << "\n";

    // Mathematical verifications:
    // R0 was loaded with 42
    assert(ctx.r[0] == 42 && "Verification Failed: R0 != 42");
    // R1 was 10 + 42 = 52, then subfunc added 5 = 57
    assert(ctx.r[1] == 57 && "Verification Failed: R1 != 57");
    // PR was saved return address 0x8C01000A
    assert(ctx.pr == 0x8C01000A && "Verification Failed: PR != 0x8C01000A");

    std::cout << "--------------------------------------------------------\n"
              << ">> ALL VERIFICATION CHECKS PASSED SUCCESSFULLY! <<\n"
              << "Static translation, delay-slot reordering & register\n"
              << "state execution verified with 100% precision.\n"
              << "========================================================\n";
    return 0;
}
