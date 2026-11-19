#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace dc::runtime {

struct PowerVRVertex {
    float x, y, z;
    float u, v;
    uint32_t color;     // ARGB8888 or packed 32-bit
    uint32_t offset_color;
};

enum class BlendMode : uint8_t {
    Opaque,
    PunchThrough,
    Translucent
};

struct PolyContext {
    BlendMode blend_mode;
    bool texture_enabled;
    uint32_t texture_address;
    uint16_t texture_width;
    uint16_t texture_height;
    bool depth_write;
    bool depth_test;
};

class PowerVRMetalRenderer {
public:
    PowerVRMetalRenderer();
    ~PowerVRMetalRenderer();

    // Initialization with native macOS window / layer (CAMetalLayer)
    bool initialize(void* metal_layer_ptr, int width, int height);
    void shutdown();

    // Frame lifecycle
    void begin_frame();
    void end_frame();

    // PowerVR Command / TA (Tile Accelerator) submission
    void submit_vertices(const PolyContext& context, const PowerVRVertex* vertices, size_t count);

    // Modern Enhancement Toggles
    void set_internal_resolution_scale(float scale); // e.g. 1.0x (640x480), 2.0x (1080p), 4.0x (4K)
    void enable_metalfx_upscaling(bool enable);
    void set_widescreen_fov_correction(bool enable);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dc::runtime
