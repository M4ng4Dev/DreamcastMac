#include "sh4_disasm.h"
#include "ir_emitter.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <string>

void print_usage(const char* prog) {
    std::cout << "DreamcastMac Static Recompiler (dc-recomp)\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  -i <file>        Input SH-4 binary (e.g. 1ST_READ.BIN)\n"
              << "  -o <file>        Output C++ source file (default: recompiled_game.cpp)\n"
              << "  -b <hex_addr>    Base load address in hex (default: 0x8C010000)\n"
              << "  -e <hex_addr>    Entry point address in hex (default: 0x8C010000)\n"
              << "  -h               Show help\n";
}

int main(int argc, char* argv[]) {
    std::string input_path;
    std::string output_path = "recompiled_game.cpp";
    uint32_t base_address = 0x8C010000;
    uint32_t entry_point = 0x8C010000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "-b" && i + 1 < argc) {
            base_address = std::stoul(argv[++i], nullptr, 16);
        } else if (arg == "-e" && i + 1 < argc) {
            entry_point = std::stoul(argv[++i], nullptr, 16);
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: No input file specified.\n\n";
        print_usage(argv[0]);
        return 1;
    }

    std::ifstream file(input_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << input_path << "\n";
        return 1;
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> binary_data(file_size);
    if (!file.read(reinterpret_cast<char*>(binary_data.data()), file_size)) {
        std::cerr << "Error: Failed to read binary data.\n";
        return 1;
    }

    std::cout << "========================================================\n"
              << " DreamcastMac - Static Binary Translator (dc-recomp)\n"
              << "========================================================\n"
              << "Input binary:   " << input_path << " (" << file_size << " bytes)\n"
              << "Base address:   0x" << std::hex << base_address << "\n"
              << "Entry point:    0x" << entry_point << std::dec << "\n"
              << "Output C++:     " << output_path << "\n"
              << "Target runtime: Apple Silicon Metal 3 (ARM64)\n"
              << "--------------------------------------------------------\n";

    std::set<uint32_t> entry_points = { entry_point };
    auto functions = dc::recomp::SH4Disassembler::analyze(
        binary_data.data(),
        binary_data.size(),
        base_address,
        entry_points
    );

    std::cout << "Disassembly & analysis complete. Discovered "
              << functions.size() << " function(s).\n";

    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open output file for writing: " << output_path << "\n";
        return 1;
    }

    dc::recomp::CppEmitter::emit_complete_translation(out_file, functions);
    std::cout << "Successfully generated translated C++ source: " << output_path << "\n";

    return 0;
}
