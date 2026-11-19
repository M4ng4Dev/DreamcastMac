#include "memory.h"
#include <cstdio>
#include <cstring>

namespace dc::runtime {

Memory g_memory;

Memory::Memory() 
    : main_ram_(MAIN_RAM_SIZE, 0),
      vram_(VRAM_SIZE, 0),
      aica_ram_(AICA_RAM_SIZE, 0) {}

void Memory::reset() {
    std::fill(main_ram_.begin(), main_ram_.end(), 0);
    std::fill(vram_.begin(), vram_.end(), 0);
    std::fill(aica_ram_.begin(), aica_ram_.end(), 0);
}

bool Memory::load_binary(uint32_t dest_addr, const uint8_t* data, size_t size) {
    uint32_t phys = dest_addr & 0x1FFFFFFF;
    // Main RAM is located at physical 0x0C000000..0x0CFFFFFF
    if (phys >= 0x0C000000 && (phys + size) <= (0x0C000000 + MAIN_RAM_SIZE)) {
        uint32_t offset = phys - 0x0C000000;
        std::memcpy(main_ram_.data() + offset, data, size);
        return true;
    }
    return false;
}

uint8_t Memory::read8(uint32_t addr) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        return main_ram_[phys & (MAIN_RAM_SIZE - 1)];
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        return vram_[phys & (VRAM_SIZE - 1)];
    }
    if ((phys & 0x1FE00000) == 0x00800000) {
        return aica_ram_[phys & (AICA_RAM_SIZE - 1)];
    }
    return static_cast<uint8_t>(read_mmio(addr, 1));
}

uint16_t Memory::read16(uint32_t addr) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        uint32_t offset = phys & (MAIN_RAM_SIZE - 2);
        uint16_t val;
        std::memcpy(&val, main_ram_.data() + offset, 2);
        return val;
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        uint32_t offset = phys & (VRAM_SIZE - 2);
        uint16_t val;
        std::memcpy(&val, vram_.data() + offset, 2);
        return val;
    }
    return static_cast<uint16_t>(read_mmio(addr, 2));
}

uint32_t Memory::read32(uint32_t addr) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        uint32_t offset = phys & (MAIN_RAM_SIZE - 4);
        uint32_t val;
        std::memcpy(&val, main_ram_.data() + offset, 4);
        return val;
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        uint32_t offset = phys & (VRAM_SIZE - 4);
        uint32_t val;
        std::memcpy(&val, vram_.data() + offset, 4);
        return val;
    }
    return read_mmio(addr, 4);
}

void Memory::write8(uint32_t addr, uint8_t val) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        main_ram_[phys & (MAIN_RAM_SIZE - 1)] = val;
        return;
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        vram_[phys & (VRAM_SIZE - 1)] = val;
        return;
    }
    if ((phys & 0x1FE00000) == 0x00800000) {
        aica_ram_[phys & (AICA_RAM_SIZE - 1)] = val;
        return;
    }
    write_mmio(addr, val, 1);
}

void Memory::write16(uint32_t addr, uint16_t val) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        uint32_t offset = phys & (MAIN_RAM_SIZE - 2);
        std::memcpy(main_ram_.data() + offset, &val, 2);
        return;
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        uint32_t offset = phys & (VRAM_SIZE - 2);
        std::memcpy(vram_.data() + offset, &val, 2);
        return;
    }
    write_mmio(addr, val, 2);
}

void Memory::write32(uint32_t addr, uint32_t val) {
    uint32_t phys = addr & 0x1FFFFFFF;
    if ((phys & 0x1C000000) == 0x0C000000) {
        uint32_t offset = phys & (MAIN_RAM_SIZE - 4);
        std::memcpy(main_ram_.data() + offset, &val, 4);
        return;
    }
    if ((phys & 0x1F800000) == 0x04000000) {
        uint32_t offset = phys & (VRAM_SIZE - 4);
        std::memcpy(vram_.data() + offset, &val, 4);
        return;
    }
    write_mmio(addr, val, 4);
}

uint32_t Memory::read_mmio(uint32_t /*addr*/, int /*size*/) {
    // MMIO stub handlers for PowerVR, Holly, Maple, GD-ROM
    // To be extended as hardware registers are hooked
    return 0;
}

void Memory::write_mmio(uint32_t /*addr*/, uint32_t /*val*/, int /*size*/) {
    // MMIO write handlers
}

} // namespace dc::runtime
