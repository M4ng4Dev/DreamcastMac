#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    uchar4 color    [[attribute(2)]];
};

struct VertexOutput {
    float4 position [[position]];
    float2 uv;
    half4  color;
    float  depth;
};

// =========================================================================
// Vertex Shader: Transforms Dreamcast 3D coordinates & applies FOV patches
// =========================================================================
vertex VertexOutput powervr_vertex_main(
    VertexInput in [[stage_in]],
    constant float4x4& projection_matrix [[buffer(1)]],
    constant float& widescreen_aspect_ratio [[buffer(2)]]
) {
    VertexOutput out;
    float4 pos = float4(in.position, 1.0);

    // Apply widescreen frustum scale if enabled
    pos.x *= widescreen_aspect_ratio;

    out.position = projection_matrix * pos;
    out.uv = in.uv;
    out.color = half4(in.color) / 255.0h;
    out.depth = out.position.z / out.position.w;
    return out;
}

// =========================================================================
// Fragment Shader: TBDR Tile-Based Rendering with Order-Independent Transparency
// =========================================================================
fragment half4 powervr_fragment_opaque(
    VertexOutput in [[stage_in]],
    texture2d<half> tex2d [[texture(0)]],
    sampler tex_sampler [[sampler(0)]]
) {
    half4 tex_color = tex2d.sample(tex_sampler, in.uv);
    return tex_color * in.color;
}

fragment half4 powervr_fragment_translucent(
    VertexOutput in [[stage_in]],
    texture2d<half> tex2d [[texture(0)]],
    sampler tex_sampler [[sampler(0)]],
    half4 current_color [[color(0), raster_order_group(0)]]
) {
    half4 tex_color = tex2d.sample(tex_sampler, in.uv) * in.color;
    half alpha = tex_color.a;

    // Direct hardware TBDR blend using Apple Silicon Tile Memory
    half4 blended;
    blended.rgb = tex_color.rgb * alpha + current_color.rgb * (1.0h - alpha);
    blended.a = current_color.a;
    return blended;
}
