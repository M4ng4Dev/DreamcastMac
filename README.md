# DreamcastMac 🌀

**High-Performance Dreamcast Static Recompiler & Native Metal Runtime for macOS (Apple Silicon)**

DreamcastMac is an ambitious project to bring SEGA Dreamcast games natively to macOS running on Apple Silicon (M1/M2/M3/M4+). Instead of relying on traditional runtime emulation layers with performance overhead, DreamcastMac leverages **Static Binary Translation (Recompilation)** and a native **Metal 3 TBDR (Tile-Based Deferred Rendering)** pipeline.

---

## 🌟 The Vision

1. **True Native Performance**: No JIT compilation hitches or translation overhead during gameplay. Dreamcast machine code (Hitachi SH-4) is statically translated into clean, optimized C++ and compiled with Clang directly into native ARM64 Mach-O binaries.
2. **The PowerVR 🤝 Apple Silicon Synergy**:
   - The Dreamcast's GPU is the **NEC/VideoLogic PowerVR CLX2 (Series2)**, which introduced **Tile-Based Deferred Rendering (TBDR)** and per-pixel Order-Independent Transparency (OIT).
   - Apple Silicon GPUs are direct architectural descendants of PowerVR's TBDR design.
   - Through **Metal 3 Tile Shaders, Imageblocks, and Memoryless Textures**, the PowerVR graphics pipeline can be mapped almost 1:1 on Apple Silicon without the heavy overhead and graphical glitches typical of PC GPUs.
3. **Modern Enhancements**:
   - **4K / 5K Liquid Retina XDR** native internal rendering.
   - **MetalFX Spatial & Temporal Upscaling** for crisp graphics with minimal thermal footprint.
   - **ProMotion (120 Hz)** support with uncoupled game-loop framerate patches.
   - **macOS Game Controller Framework** for plug-and-play support for PS5 DualSense, Xbox Wireless, and Switch Pro controllers.
   - **Widescreen (16:9 / 16:10 / 21:9)** FOV and geometry frustum patches.

---

## 🏗️ Architecture

```
DreamcastMac/
├── CMakeLists.txt              # Root build configuration
├── tools/
│   └── recompiler/             # dc-recomp: Static Binary Translator CLI
│       ├── include/            # Disassembler & AST definitions
│       │   ├── sh4_disasm.h
│       │   ├── sh4_opcodes.h
│       │   └── ir_emitter.h
│       └── src/                # Implementation of binary analysis & C++ code generation
│           ├── main.cpp
│           ├── sh4_disasm.cpp
│           └── ir_emitter.cpp
├── runtime/                    # Shared runtime linked with all recompiled games
│   ├── include/
│   │   ├── sh4_context.h       # CPU registers, flags, and execution state
│   │   ├── sh4_instructions.h  # Optimized C++ / ARM64 NEON instruction implementations
│   │   ├── memory.h            # Dreamcast 16MB/8MB/2MB memory map & MMIO handlers
│   │   ├── powervr_metal.h     # PowerVR CLX2 -> Metal 3 graphics interface
│   │   ├── maple_bus.h         # Controller, VMU, and peripheral subsystem
│   │   └── aica_audio.h        # Yamaha AICA (ARM7 + DSP) sound subsystem
│   └── src/
│       ├── memory.cpp
│       ├── maple_bus.cpp
│       └── powervr_metal.mm    # Metal backend & tile shaders (macOS)
└── samples/                    # Test roms, pilot demos, and verification benchmarks
    └── sh4_test/
```

---

## 🚀 The Recompilation Pipeline

```
┌─────────────────────────┐
│   1ST_READ.BIN (SH-4)   │  (Dreamcast game executable)
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│     dc-recomp Tool      │  (Static Binary Translator)
│  • Control Flow Graph   │
│  • Function Detection   │
│  • Delay Slot Handling  │
│  • C++ Code Generation  │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│  game_recompiled.cpp    │  (Translated C++ functions)
└────────────┬────────────┘
             │  + linked with:
             ▼
┌─────────────────────────┐
│     dc-metal-runtime    │  (Metal 3 GPU, CoreAudio, GameController)
└────────────┬────────────┘
             │  Compiled with Xcode / Clang for arm64-apple-macos
             ▼
┌─────────────────────────┐
│     GameName.app        │  (Standalone, ultra-fast native macOS Application!)
└─────────────────────────┘
```

---

## 🛠️ Building the Project

### Requirements:
- **macOS** with Apple Silicon (M1/M2/M3/M4 or later)
- **Xcode 15+** with Command Line Tools
- **CMake 3.25+** and **Ninja**
- *(Optional for cross-development)*: Linux / Debian with GCC 12+ or Clang 14+

### Build Instructions:
```bash
mkdir build && cd build
cmake -G Ninja ..
ninja
```

---

## 🗺️ Roadmap & Milestones

- [x] **Milestone 0**: Repository layout, architecture design, and specifications.
- [ ] **Milestone 1**: `dc-recomp` SH-4 complete disassembler and basic block analyzer.
- [ ] **Milestone 2**: Static C++ code emitter with register mapping and delay-slot normalization.
- [ ] **Milestone 3**: Runtime memory architecture (16MB Main RAM, mirrors, MMIO dispatch).
- [ ] **Milestone 4**: Pilot executable test: Recompile a Dreamcast homebrew binary and run it natively.
- [ ] **Milestone 5**: Metal 3 TBDR pipeline implementation (Tile Shaders, ISP/TSP emulation).
- [ ] **Milestone 6**: First commercial pilot game port (Crazy Taxi or Ikaruga target).
- [ ] **Milestone 7**: MetalFX Upscaling, ProMotion 120Hz uncoupling, and UI wrapper.
