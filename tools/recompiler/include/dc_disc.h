#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace dc::disc {

// Descrambles a Dreamcast 1ST_READ.BIN using Marcus Comstedt's standard algorithm.
std::vector<uint8_t> descramble_1st_read(const std::vector<uint8_t>& scrambled);

// Heuristic to detect if a binary is likely scrambled or already unscrambled.
bool is_likely_scrambled(const std::vector<uint8_t>& data);

// Extracts a named file (e.g. "1ST_READ.BIN") from a .gdi, .iso, or raw .bin/.cdi track.
bool extract_file_from_disc(
    const std::string& disc_path,
    const std::string& target_filename,
    std::vector<uint8_t>& out_data,
    std::string& out_error
);

} // namespace dc::disc
