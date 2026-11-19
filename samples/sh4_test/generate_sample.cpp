#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int main() {
    // Generate a valid SH-4 test binary:
    // 0x8C010000: mov #42, r0        (0xE02A)
    // 0x8C010002: mov #10, r1        (0xE10A)
    // 0x8C010004: add r0, r1         (0x310C)  -> r1 = 52
    // 0x8C010006: bsr 0x8C010010     (0xB003)  -> call subfunc
    // 0x8C010008: nop                (0x0009)  -> delay slot
    // 0x8C01000A: rts                (0x000B)  -> return
    // 0x8C01000C: nop                (0x0009)  -> delay slot
    // 0x8C01000E: nop                (0x0009)  -> alignment
    // Subfunc @ 0x8C010010:
    // 0x8C010010: add #5, r1         (0x7105)  -> r1 = 57
    // 0x8C010012: rts                (0x000B)
    // 0x8C010014: nop                (0x0009)

    std::vector<uint16_t> opcodes = {
        0xE02A, // mov #42, r0
        0xE10A, // mov #10, r1
        0x310C, // add r0, r1
        0xB003, // bsr +8 (to 0x8C010010)
        0x0009, // nop (delay slot)
        0x000B, // rts
        0x0009, // nop (delay slot)
        0x0009, // nop padding
        // 0x8C010010:
        0x7105, // add #5, r1
        0x000B, // rts
        0x0009  // nop (delay slot)
    };

    std::ofstream out("sample_sh4.bin", std::ios::binary);
    for (uint16_t op : opcodes) {
        uint8_t lo = op & 0xFF;
        uint8_t hi = (op >> 8) & 0xFF;
        out.write(reinterpret_cast<char*>(&lo), 1);
        out.write(reinterpret_cast<char*>(&hi), 1);
    }
    out.close();

    std::cout << "Generated sample_sh4.bin with " << (opcodes.size() * 2) << " bytes.\n";
    return 0;
}
