#include "dc_disc.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

namespace dc::disc {

namespace {

constexpr size_t MAX_CHUNK = 2048 * 1024; // 2 MB

class DescrambleRNG {
public:
    explicit DescrambleRNG(size_t total_size)
        : seed_(static_cast<uint32_t>(total_size & 0xFFFF)) {}

    uint32_t next() noexcept {
        seed_ = (seed_ * 2109 + 9273) & 0x7FFF;
        return (seed_ + 0xC000) & 0xFFFF;
    }

private:
    uint32_t seed_;
};

void descramble_chunk(
    const uint8_t* src,
    uint8_t* dst,
    size_t chunk_bytes,
    DescrambleRNG& rng
) {
    size_t slices = chunk_bytes / 32;
    if (slices == 0) return;

    std::vector<size_t> idx(slices);
    for (size_t i = 0; i < slices; ++i) {
        idx[i] = i;
    }

    for (size_t i = slices; i > 0; --i) {
        size_t pos = i - 1;
        size_t x = (rng.next() * pos) >> 16;
        std::swap(idx[pos], idx[x]);
        std::memcpy(dst + 32 * idx[pos], src, 32);
        src += 32;
    }
}

std::string to_upper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string strip_iso_version(const std::string& name) {
    auto semi = name.find(';');
    if (semi != std::string::npos) {
        return name.substr(0, semi);
    }
    return name;
}

struct DiscTrack {
    int number = 0;
    uint32_t start_lba = 0;
    int type = 0;
    int sector_size = 2048;
    std::string filename;
};

struct SectorFormat {
    size_t sector_size = 2048;
    size_t data_offset = 0;
};

bool detect_pvd_in_file(
    std::ifstream& file,
    size_t file_size,
    size_t& out_pvd_file_offset,
    SectorFormat& out_format
) {
    // Scan for ISO9660 Primary Volume Descriptor signature: 0x01 "CD001" 0x01
    static const uint8_t PVD_SIG[7] = { 0x01, 'C', 'D', '0', '0', '1', 0x01 };
    const size_t scan_limit = std::min<size_t>(file_size, 64 * 1024 * 1024);
    std::vector<uint8_t> buf(4 * 1024 * 1024);

    // Search backwards or forwards; in Dreamcast multi-session discs, the last session is the data track
    size_t last_match = SIZE_MAX;
    size_t pos = 0;
    file.seekg(0, std::ios::beg);

    while (pos < scan_limit) {
        size_t to_read = std::min(buf.size(), scan_limit - pos);
        file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(to_read));
        if (to_read < 7) break;

        for (size_t i = 0; i + 7 <= to_read; ++i) {
            if (std::memcmp(buf.data() + i, PVD_SIG, 7) == 0) {
                last_match = pos + i;
            }
        }
        if (to_read == buf.size()) {
            pos += to_read - 16;
            file.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
        } else {
            break;
        }
    }

    if (last_match == SIZE_MAX) {
        return false;
    }

    out_pvd_file_offset = last_match;
    // Determine sector size and data offset from alignment
    if ((last_match % 2352) == 24) {
        out_format = { 2352, 24 }; // Mode 2 XA Form 1
    } else if ((last_match % 2352) == 16) {
        out_format = { 2352, 16 }; // Mode 1 raw
    } else if ((last_match % 2336) == 8) {
        out_format = { 2336, 8 };  // Mode 2 2336
    } else {
        out_format = { 2048, 0 };  // Standard ISO 2048
    }
    return true;
}

bool read_sector(
    std::ifstream& file,
    size_t file_sector_index,
    const SectorFormat& fmt,
    uint8_t* out_2048
) {
    std::streamoff offset = static_cast<std::streamoff>(file_sector_index * fmt.sector_size + fmt.data_offset);
    file.clear();
    file.seekg(offset, std::ios::beg);
    if (!file) return false;
    file.read(reinterpret_cast<char*>(out_2048), 2048);
    return file.gcount() == 2048;
}

uint32_t read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool extract_from_binary_image(
    const std::string& bin_path,
    uint32_t track_start_lba,
    bool has_explicit_lba,
    const std::string& target_filename,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    std::ifstream file(bin_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        out_error = "Could not open image file: " + bin_path;
        return false;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    size_t pvd_offset = 0;
    SectorFormat fmt{};

    if (!detect_pvd_in_file(file, file_size, pvd_offset, fmt)) {
        out_error = "Could not find ISO9660 Primary Volume Descriptor (CD001) in " + bin_path;
        return false;
    }

    size_t pvd_file_sector = (pvd_offset - fmt.data_offset) / fmt.sector_size;
    uint8_t pvd[2048];
    if (!read_sector(file, pvd_file_sector, fmt, pvd)) {
        out_error = "Failed to read PVD sector.";
        return false;
    }

    // Root directory record is at PVD offset 156
    uint32_t root_lba = read_u32_le(pvd + 156 + 2);
    uint32_t root_size = read_u32_le(pvd + 156 + 10);

    // Compute LBA -> file_sector mapping
    int64_t lba_bias = 0;
    if (has_explicit_lba) {
        // Track starts at track_start_lba -> file sector 0
        lba_bias = -static_cast<int64_t>(track_start_lba);
    } else {
        // PVD is always at session_start_lba + 16; find root directory sector near PVD
        size_t total_sectors = file_size / fmt.sector_size;
        bool found_root = false;
        uint8_t sec[2048];
        size_t search_start = pvd_file_sector;
        size_t search_end = std::min(total_sectors, pvd_file_sector + 256);

        for (size_t s = search_start; s < search_end; ++s) {
            if (!read_sector(file, s, fmt, sec)) break;
            // ISO9660 '.' entry has len >= 34, name_len == 1, name == 0x00, extent_lba == root_lba
            if (sec[0] >= 34 && sec[32] == 1 && sec[33] == 0x00 && read_u32_le(sec + 2) == root_lba) {
                lba_bias = static_cast<int64_t>(s) - static_cast<int64_t>(root_lba);
                found_root = true;
                break;
            }
        }
        if (!found_root) {
            // Fallback: assume PVD is at session_lba + 16 and root is relative
            lba_bias = static_cast<int64_t>(pvd_file_sector) - static_cast<int64_t>(root_lba - 2);
        }
    }

    std::string target_upper = to_upper(target_filename);
    uint32_t dir_sectors = (root_size + 2047) / 2048;
    std::vector<uint8_t> dir_buf(dir_sectors * 2048);

    for (uint32_t i = 0; i < dir_sectors; ++i) {
        int64_t fsec = static_cast<int64_t>(root_lba + i) + lba_bias;
        if (fsec < 0 || !read_sector(file, static_cast<size_t>(fsec), fmt, dir_buf.data() + i * 2048)) {
            out_error = "Failed to read root directory sector.";
            return false;
        }
    }

    size_t offset = 0;
    while (offset < dir_buf.size()) {
        uint8_t rec_len = dir_buf[offset];
        if (rec_len == 0) {
            // Jump to next 2048-byte sector boundary
            offset = ((offset / 2048) + 1) * 2048;
            continue;
        }

        uint32_t file_lba = read_u32_le(dir_buf.data() + offset + 2);
        uint32_t file_len = read_u32_le(dir_buf.data() + offset + 10);
        uint8_t flags = dir_buf[offset + 25];
        uint8_t name_len = dir_buf[offset + 32];

        if ((flags & 0x02) == 0 && name_len > 0 && offset + 33 + name_len <= dir_buf.size()) {
            std::string entry_name(reinterpret_cast<const char*>(dir_buf.data() + offset + 33), name_len);
            entry_name = to_upper(strip_iso_version(entry_name));

            if (entry_name == target_upper) {
                out_data.resize(file_len);
                uint32_t num_secs = (file_len + 2047) / 2048;
                uint8_t sec[2048];
                for (uint32_t s = 0; s < num_secs; ++s) {
                    int64_t fsec = static_cast<int64_t>(file_lba + s) + lba_bias;
                    if (fsec < 0 || !read_sector(file, static_cast<size_t>(fsec), fmt, sec)) {
                        out_error = "Failed reading file sectors for " + target_filename;
                        return false;
                    }
                    size_t copy_len = std::min<size_t>(2048, file_len - s * 2048);
                    std::memcpy(out_data.data() + s * 2048, sec, copy_len);
                }
                return true;
            }
        }

        offset += rec_len;
    }

    out_error = "File '" + target_filename + "' not found in root directory of " + bin_path;
    return false;
}

} // namespace

std::vector<uint8_t> descramble_1st_read(const std::vector<uint8_t>& scrambled) {
    std::vector<uint8_t> out(scrambled.size());
    DescrambleRNG rng(scrambled.size());

    size_t remaining = scrambled.size();
    const uint8_t* src = scrambled.data();
    uint8_t* dst = out.data();

    for (size_t chunk_sz = MAX_CHUNK; chunk_sz >= 32; chunk_sz >>= 1) {
        while (remaining >= chunk_sz) {
            descramble_chunk(src, dst, chunk_sz, rng);
            src += chunk_sz;
            dst += chunk_sz;
            remaining -= chunk_sz;
        }
    }

    if (remaining > 0) {
        std::memcpy(dst, src, remaining);
    }

    return out;
}

bool is_likely_scrambled(const std::vector<uint8_t>& data) {
    if (data.size() < 256) return false;

    // Count common SH-4 opcodes (NOP 0x0009, RTS 0x000B, STS.L PR,@-R15 0x4F22, MOV.L @R15+,...)
    auto score_sh4 = [](const std::vector<uint8_t>& buf) -> int {
        int score = 0;
        size_t limit = std::min<size_t>(buf.size(), 8192);
        for (size_t i = 0; i + 1 < limit; i += 2) {
            uint16_t op = static_cast<uint16_t>(buf[i] | (buf[i + 1] << 8));
            if (op == 0x0009 || op == 0x000B || op == 0x4F22 || op == 0x4F26) {
                score += 4;
            }
            // mov.l @(disp, PC), Rn (0xDxxx)
            if ((op & 0xF000) == 0xD000) {
                score += 1;
            }
        }
        return score;
    };

    int raw_score = score_sh4(data);
    std::vector<uint8_t> descrambled = descramble_1st_read(data);
    int descrambled_score = score_sh4(descrambled);

    return descrambled_score > raw_score * 2;
}

bool extract_file_from_disc(
    const std::string& disc_path,
    const std::string& target_filename,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    std::string upper_path = to_upper(disc_path);
    if (upper_path.size() >= 4 && upper_path.substr(upper_path.size() - 4) == ".GDI") {
        std::ifstream gdi(disc_path);
        if (!gdi.is_open()) {
            out_error = "Could not open .gdi file: " + disc_path;
            return false;
        }

        int track_count = 0;
        gdi >> track_count;
        std::string line;
        std::getline(gdi, line);

        std::vector<DiscTrack> tracks;
        while (std::getline(gdi, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            DiscTrack t;
            int offset_val = 0;
            if (iss >> t.number >> t.start_lba >> t.type >> t.sector_size) {
                std::string rest;
                std::getline(iss, rest);
                // Extract filename (may be quoted)
                auto first_q = rest.find('"');
                if (first_q != std::string::npos) {
                    auto last_q = rest.find('"', first_q + 1);
                    if (last_q != std::string::npos) {
                        t.filename = rest.substr(first_q + 1, last_q - first_q - 1);
                    }
                } else {
                    std::istringstream riss(rest);
                    riss >> t.filename >> offset_val;
                }
                if (!t.filename.empty()) {
                    tracks.push_back(t);
                }
            }
        }

        // Resolve directory of the .gdi file
        std::string base_dir;
        auto slash = disc_path.find_last_of("/\\");
        if (slash != std::string::npos) {
            base_dir = disc_path.substr(0, slash + 1);
        }

        // Try data tracks (type == 4) in reverse order (last data track holds the game filesystem)
        for (auto it = tracks.rbegin(); it != tracks.rend(); ++it) {
            if (it->type == 4) {
                std::string track_path = base_dir + it->filename;
                if (extract_from_binary_image(track_path, it->start_lba, true, target_filename, out_data, out_error)) {
                    return true;
                }
            }
        }
        out_error = "Could not locate '" + target_filename + "' in any data track of " + disc_path;
        return false;
    }

    // Otherwise treat as .iso / .cdi / .bin
    return extract_from_binary_image(disc_path, 0, false, target_filename, out_data, out_error);
}

} // namespace dc::disc
