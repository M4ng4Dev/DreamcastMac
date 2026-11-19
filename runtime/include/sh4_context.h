#pragma once
#include <cstdint>
#include <cstring>

namespace dc::runtime {

// Status Register (SR) bit definitions
enum SRBits : uint32_t {
    SR_MD = (1u << 30), // Processor Mode (1 = Privileged)
    SR_RB = (1u << 29), // Register Bank (1 = Bank 1 active for R0-R7)
    SR_BL = (1u << 28), // Block Interrupts
    SR_FD = (1u << 15), // FPU Disable
    SR_M  = (1u << 9),  // Divide Step Flag M
    SR_Q  = (1u << 8),  // Divide Step Flag Q
    SR_IMASK = (0xFu << 4), // Interrupt Mask
    SR_S  = (1u << 1),  // Saturation Flag
    SR_T  = (1u << 0)   // Condition / Carry Flag T
};

// Floating-Point Status/Control Register (FPSCR) bit definitions
enum FPSCRBits : uint32_t {
    FPSCR_FR = (1u << 21), // Float Register Bank (0 = FR0-15, 1 = XF0-15)
    FPSCR_SZ = (1u << 20), // Transfer Size Mode (0 = 32-bit, 1 = 64-bit pair)
    FPSCR_PR = (1u << 19), // Precision Mode (0 = Single, 1 = Double)
    FPSCR_DN = (1u << 18), // Denormalization Mode (1 = Flush to zero)
    FPSCR_RM = (0x3u << 0) // Rounding Mode (0 = Nearest, 1 = Toward zero)
};

// SH-4 CPU Execution Context
struct alignas(64) SH4Context {
    // General Purpose Registers (32-bit)
    uint32_t r[16];

    // Banked Registers (R0_BANK - R7_BANK)
    uint32_t r_bank[8];

    // Control and System Registers
    uint32_t pc;   // Program Counter
    uint32_t pr;   // Procedure Register (Return Address)
    uint32_t sr;   // Status Register
    uint32_t gbr;  // Global Base Register
    uint32_t vbr;  // Vector Base Register
    uint32_t mach; // Multiply-Accumulate High
    uint32_t macl; // Multiply-Accumulate Low

    uint32_t ssr;  // Saved Status Register
    uint32_t spc;  // Saved Program Counter
    uint32_t sgr;  // Saved General Register
    uint32_t dbr;  // Debug Base Register

    // Floating Point Unit
    uint32_t fpscr; // FPU Status / Control
    uint32_t fpul;  // FPU Communication Register

    // Single-precision registers (FR0-FR15 and XF0-XF15)
    union {
        float f[16];
        uint32_t u[16];
    } fr;

    union {
        float f[16];
        uint32_t u[16];
    } xf;

    // Execution control flags
    bool halted;
    uint32_t cycles_remaining;

    // Fast-path flag accessors
    [[nodiscard]] inline bool get_t() const noexcept {
        return (sr & SR_T) != 0;
    }

    inline void set_t(bool condition) noexcept {
        if (condition) {
            sr |= SR_T;
        } else {
            sr &= ~SR_T;
        }
    }

    [[nodiscard]] inline bool get_s() const noexcept {
        return (sr & SR_S) != 0;
    }

    inline void set_s(bool condition) noexcept {
        if (condition) {
            sr |= SR_S;
        } else {
            sr &= ~SR_S;
        }
    }

    // Register Bank Switch helper (MD=1, RB toggle)
    inline void switch_register_bank(bool to_bank1) noexcept {
        if (to_bank1 == ((sr & SR_RB) != 0)) return;

        for (int i = 0; i < 8; ++i) {
            uint32_t temp = r[i];
            r[i] = r_bank[i];
            r_bank[i] = temp;
        }

        if (to_bank1) {
            sr |= SR_RB;
        } else {
            sr &= ~SR_RB;
        }
    }

    void reset(uint32_t initial_pc) noexcept {
        std::memset(this, 0, sizeof(SH4Context));
        pc = initial_pc;
        sr = SR_MD | SR_BL | SR_RB; // Reset default: Privileged, interrupts blocked, bank 1
        fpscr = 0x00040001;          // Default: Single precision, round nearest
    }
};

} // namespace dc::runtime
