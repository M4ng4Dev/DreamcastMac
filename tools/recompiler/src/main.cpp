#include "sh4_disasm.h"
#include "ir_emitter.h"
#include "dc_disc.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

void print_usage(const char* prog) {
    std::cout << "DreamcastMac Static Recompiler (dc-recomp)\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  -i <file>        Input SH-4 binary (1ST_READ.BIN) OR disc image (.gdi, .iso, .cdi)\n"
              << "  -o <file>        Output C++ source file (default: recompiled_game.cpp)\n"
              << "  -b <hex_addr>    Base load address in hex (default: 0x8C010000)\n"
              << "  -e <hex_addr>    Entry point address in hex (default: 0x8C010000)\n"
              << "  --descramble     Force Dreamcast 1ST_READ.BIN descrambling\n"
              << "  --no-descramble  Disable automatic scramble detection\n"
              << "  --dump-bin <f>   Save extracted/descrambled 1ST_READ.BIN to file\n"
              << "  -h, --help       Show help\n";
}

static bool has_disc_extension(const std::string& path) {
    if (path.size() < 4) return false;
    std::string ext = path.substr(path.size() - 4);
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".gdi" || ext == ".iso" || ext == ".cdi";
}

int main(int argc, char* argv[]) {
    std::string input_path;
    std::string output_path = "recompiled_game.cpp";
    std::string dump_bin_path;
    uint32_t base_address = 0x8C010000;
    uint32_t entry_point = 0x8C010000;
    int descramble_mode = 0; // 0 = auto, 1 = force, -1 = disable

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
        } else if (arg == "--descramble") {
            descramble_mode = 1;
        } else if (arg == "--no-descramble") {
            descramble_mode = -1;
        } else if (arg == "--dump-bin" && i + 1 < argc) {
            dump_bin_path = argv[++i];
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

    std::vector<uint8_t> binary_data;
    if (has_disc_extension(input_path)) {
        std::cout << "[Disc] Extracting 1ST_READ.BIN from " << input_path << "...\n";
        std::string err;
        if (!dc::disc::extract_file_from_disc(input_path, "1ST_READ.BIN", binary_data, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "[Disc] Extracted 1ST_READ.BIN (" << binary_data.size() << " bytes).\n";
    } else {
        std::ifstream file(input_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open input file: " << input_path << "\n";
            return 1;
        }
        size_t file_size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        binary_data.resize(file_size);
        if (!file.read(reinterpret_cast<char*>(binary_data.data()), static_cast<std::streamsize>(file_size))) {
            std::cerr << "Error: Failed to read binary data.\n";
            return 1;
        }
    }

    bool do_descramble = false;
    if (descramble_mode == 1) {
        do_descramble = true;
    } else if (descramble_mode == 0 && dc::disc::is_likely_scrambled(binary_data)) {
        std::cout << "[Auto-Detect] Binary appears scrambled; applying Dreamcast descramble...\n";
        do_descramble = true;
    }

    if (do_descramble) {
        binary_data = dc::disc::descramble_1st_read(binary_data);
        std::cout << "[Descramble] Completed.\n";
    }

    if (!dump_bin_path.empty()) {
        std::ofstream dump(dump_bin_path, std::ios::binary);
        if (dump.is_open()) {
            dump.write(reinterpret_cast<const char*>(binary_data.data()), static_cast<std::streamsize>(binary_data.size()));
            std::cout << "[Dump] Saved clean 1ST_READ.BIN to " << dump_bin_path << "\n";
        }
    }

    std::cout << "========================================================\n"
              << " DreamcastMac - Static Binary Translator (dc-recomp)\n"
              << "========================================================\n"
              << "Input source:   " << input_path << " (" << binary_data.size() << " bytes)\n"
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

    size_t total_blocks = 0;
    size_t total_insts = 0;
    size_t known_insts = 0;
    std::map<uint16_t, size_t> unknown_counts;

    for (const auto& f : functions) {
        total_blocks += f.blocks.size();
        for (const auto& bb : f.blocks) {
            for (const auto& inst : bb.instructions) {
                ++total_insts;
                if (inst.type != dc::recomp::OpcodeType::UNKNOWN) {
                    ++known_insts;
                } else {
                    unknown_counts[inst.raw_opcode]++;
                }
            }
        }
    }

    double coverage = total_insts > 0 ? (100.0 * static_cast<double>(known_insts) / static_cast<double>(total_insts)) : 0.0;
    std::cout << "Discovered " << functions.size() << " function(s), "
              << total_blocks << " basic block(s), "
              << total_insts << " instruction(s).\n"
              << "Opcode coverage: " << known_insts << "/" << total_insts
              << " (" << std::fixed << std::setprecision(1) << coverage << "%)\n";

    if (!unknown_counts.empty()) {
        std::vector<std::pair<size_t, uint16_t>> sorted_unknowns;
        sorted_unknowns.reserve(unknown_counts.size());
        for (const auto& kv : unknown_counts) {
            sorted_unknowns.push_back({ kv.second, kv.first });
        }
        std::sort(sorted_unknowns.rbegin(), sorted_unknowns.rend());

        std::cout << "Top unknown opcodes:\n";
        size_t limit = std::min<size_t>(10, sorted_unknowns.size());
        for (size_t i = 0; i < limit; ++i) {
            std::cout << "  0x" << std::hex << std::setfill('0') << std::setw(4)
                      << sorted_unknowns[i].second << std::dec
                      << " (" << sorted_unknowns[i].first << "x)\n";
        }
    }

    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open output file for writing: " << output_path << "\n";
        return 1;
    }

    dc::recomp::CppEmitter::emit_complete_translation(out_file, functions);
    std::cout << "Successfully generated translated C++ source: " << output_path << "\n";

    return 0;
}
