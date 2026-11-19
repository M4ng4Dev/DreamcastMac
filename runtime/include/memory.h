#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace dc::runtime {

constexpr size_t MAIN_RAM_SIZE  = 16 * 1024 * 1024; // 16 MB
constexpr size_t VRAM_SIZE      = 8  * 1024 * 1024; // 8 MB
constexpr size_t AICA_RAM_SIZE  = 2  * 1024 * 1024; // 2 MB

class Memory {
public:
    Memory();
    ~Memory() = default;

    void reset();

    // Fast-path Direct Pointers
    [[nodiscard]] inline uint8_t* get_main_ram() noexcept { return main_ram_.data(); }
    [[nodiscard]] inline const uint8_t* get_main_ram() const noexcept { return main_ram_.data(); }

    [[nodiscard]] inline uint8_t* get_vram() noexcept { return vram_.data(); }
    [[nodiscard]] inline uint8_t* get_aica_ram() noexcept { return aica_ram_.data(); }

    // Memory read operations (Handles P0/P1/P2/P3 mirroring & MMIO)
    uint8_t  read8(uint32_t addr);
    uint16_t read16(uint32_t addr);
    uint32_t read32(uint32_t addr);

    // Memory write operations
    void write8(uint32_t addr, uint8_t val);
    void write16(uint32_t addr, uint16_t val);
    void write32(uint32_t addr, uint32_t val);

    // Bulk loader (e.g. loading 1ST_READ.BIN to 0x8C010000)
    bool load_binary(uint32_t dest_addr, const uint8_t* data, size_t size);

private:
    std::vector<uint8_t> main_ram_;
    std::vector<uint8_t> vram_;
    std::vector<uint8_t> aica_ram_;

    // MMIO Dispatches
    uint32_t read_mmio(uint32_t addr, int size);
    void write_mmio(uint32_t addr, uint32_t val, int size);
};

// Global or thread-local runtime memory instance
extern Memory g_memory;

// Inlined fast memory access for recompiled code targeting Main RAM (0x8C000000 - 0x8CFFFFFF)
inline uint32_t fast_read32_ram(uint32_t addr) noexcept {
    uint32_t offset = addr & (MAIN_RAM_SIZE - 1);
    const uint8_t* ptr = g_memory.get_main_ram() + offset;
    uint32_t val;
    __builtin_memcpy(&val, ptr, sizeof(val));
    return val;
}

inline void fast_write32_ram(uint32_t addr, uint32_t val) noexcept {
    uint32_t offset = addr & (MAIN_RAM_SIZE - 1);
    uint8_t* ptr = g_memory.get_main_ram() + offset;
    __builtin_memcpy(ptr, &val, sizeof(val));
}

} // namespace dc::runtime
