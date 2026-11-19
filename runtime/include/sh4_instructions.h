#pragma once
#include "sh4_context.h"
#include <cmath>
#include <cstdint>

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace dc::runtime::ops {

// Integer Add / Subtract with Carry / Overflow
inline void add(SH4Context& ctx, int m, int n) noexcept {
    ctx.r[n] += ctx.r[m];
}

inline void addc(SH4Context& ctx, int m, int n) noexcept {
    uint32_t carry = ctx.get_t() ? 1 : 0;
    uint32_t rn = ctx.r[n];
    uint32_t rm = ctx.r[m];
    uint64_t sum = static_cast<uint64_t>(rn) + rm + carry;
    ctx.r[n] = static_cast<uint32_t>(sum);
    ctx.set_t((sum >> 32) != 0);
}

inline void sub(SH4Context& ctx, int m, int n) noexcept {
    ctx.r[n] -= ctx.r[m];
}

inline void subc(SH4Context& ctx, int m, int n) noexcept {
    uint32_t borrow = ctx.get_t() ? 1 : 0;
    uint32_t rn = ctx.r[n];
    uint32_t rm = ctx.r[m];
    uint64_t diff = static_cast<uint64_t>(rn) - rm - borrow;
    ctx.r[n] = static_cast<uint32_t>(diff);
    ctx.set_t((diff >> 32) != 0);
}

// Compare Instructions
inline void cmp_eq(SH4Context& ctx, int m, int n) noexcept {
    ctx.set_t(ctx.r[n] == ctx.r[m]);
}

inline void cmp_ge(SH4Context& ctx, int m, int n) noexcept {
    ctx.set_t(static_cast<int32_t>(ctx.r[n]) >= static_cast<int32_t>(ctx.r[m]));
}

inline void cmp_gt(SH4Context& ctx, int m, int n) noexcept {
    ctx.set_t(static_cast<int32_t>(ctx.r[n]) > static_cast<int32_t>(ctx.r[m]));
}

inline void cmp_hs(SH4Context& ctx, int m, int n) noexcept {
    ctx.set_t(ctx.r[n] >= ctx.r[m]);
}

inline void cmp_hi(SH4Context& ctx, int m, int n) noexcept {
    ctx.set_t(ctx.r[n] > ctx.r[m]);
}

// Multiply / Divide
inline void mulu_w(SH4Context& ctx, int m, int n) noexcept {
    ctx.macl = (ctx.r[n] & 0xFFFF) * (ctx.r[m] & 0xFFFF);
}

inline void muls_w(SH4Context& ctx, int m, int n) noexcept {
    int16_t rn = static_cast<int16_t>(ctx.r[n]);
    int16_t rm = static_cast<int16_t>(ctx.r[m]);
    ctx.macl = static_cast<uint32_t>(static_cast<int32_t>(rn) * rm);
}

inline void dmulu_l(SH4Context& ctx, int m, int n) noexcept {
    uint64_t result = static_cast<uint64_t>(ctx.r[n]) * static_cast<uint64_t>(ctx.r[m]);
    ctx.macl = static_cast<uint32_t>(result);
    ctx.mach = static_cast<uint32_t>(result >> 32);
}

inline void dmuls_l(SH4Context& ctx, int m, int n) noexcept {
    int64_t result = static_cast<int64_t>(static_cast<int32_t>(ctx.r[n])) * static_cast<int64_t>(static_cast<int32_t>(ctx.r[m]));
    ctx.macl = static_cast<uint32_t>(result);
    ctx.mach = static_cast<uint32_t>(result >> 32);
}

// =========================================================================
// Floating-Point & Vector Instructions (Accelerated for Apple Silicon ARM64)
// =========================================================================

inline void fadd(SH4Context& ctx, int m, int n) noexcept {
    ctx.fr.f[n] += ctx.fr.f[m];
}

inline void fsub(SH4Context& ctx, int m, int n) noexcept {
    ctx.fr.f[n] -= ctx.fr.f[m];
}

inline void fmul(SH4Context& ctx, int m, int n) noexcept {
    ctx.fr.f[n] *= ctx.fr.f[m];
}

inline void fdiv(SH4Context& ctx, int m, int n) noexcept {
    ctx.fr.f[n] /= ctx.fr.f[m];
}

inline void fsqrt(SH4Context& ctx, int n) noexcept {
    ctx.fr.f[n] = std::sqrt(ctx.fr.f[n]);
}

/**
 * FIPR: 4D Vector Inner Product (Dot Product)
 * FVn[3] = FVm[0]*FVn[0] + FVm[1]*FVn[1] + FVm[2]*FVn[2] + FVm[3]*FVn[3]
 * In SH-4, vector registers are groups of 4: FV0 (0-3), FV4 (4-7), FV8 (8-11), FV12 (12-15)
 */
inline void fipr(SH4Context& ctx, int m, int n) noexcept {
#if defined(__ARM_NEON) || defined(__aarch64__)
    float32x4_t vm = vld1q_f32(&ctx.fr.f[m]);
    float32x4_t vn = vld1q_f32(&ctx.fr.f[n]);
    float32x4_t prod = vmulq_f32(vm, vn);
#if defined(__aarch64__)
    ctx.fr.f[n + 3] = vaddvq_f32(prod);
#else
    float32x2_t sum2 = vadd_f32(vget_low_f32(prod), vget_high_f32(prod));
    ctx.fr.f[n + 3] = vget_lane_f32(vpadd_f32(sum2, sum2), 0);
#endif
#else
    // Fallback scalar implementation
    float dot = ctx.fr.f[m + 0] * ctx.fr.f[n + 0]
              + ctx.fr.f[m + 1] * ctx.fr.f[n + 1]
              + ctx.fr.f[m + 2] * ctx.fr.f[n + 2]
              + ctx.fr.f[m + 3] * ctx.fr.f[n + 3];
    ctx.fr.f[n + 3] = dot;
#endif
}

/**
 * FTRV: 4D Vector Transform by 4x4 Matrix
 * Transforms vector FVn by matrix stored in background float registers XF0-XF15:
 *   [ XF0  XF4  XF8  XF12 ] [ FVn[0] ]
 *   [ XF1  XF5  XF9  XF13 ] [ FVn[1] ]
 *   [ XF2  XF6  XF10 XF14 ] [ FVn[2] ]
 *   [ XF3  XF7  XF11 XF15 ] [ FVn[3] ]
 */
inline void ftrv(SH4Context& ctx, int n) noexcept {
#if defined(__ARM_NEON) || defined(__aarch64__)
    float v0 = ctx.fr.f[n + 0];
    float v1 = ctx.fr.f[n + 1];
    float v2 = ctx.fr.f[n + 2];
    float v3 = ctx.fr.f[n + 3];

    float32x4_t col0 = vld1q_f32(&ctx.xf.f[0]);
    float32x4_t col1 = vld1q_f32(&ctx.xf.f[4]);
    float32x4_t col2 = vld1q_f32(&ctx.xf.f[8]);
    float32x4_t col3 = vld1q_f32(&ctx.xf.f[12]);

    float32x4_t res = vmulq_n_f32(col0, v0);
    res = vmlaq_n_f32(res, col1, v1);
    res = vmlaq_n_f32(res, col2, v2);
    res = vmlaq_n_f32(res, col3, v3);

    vst1q_f32(&ctx.fr.f[n], res);
#else
    float v0 = ctx.fr.f[n + 0];
    float v1 = ctx.fr.f[n + 1];
    float v2 = ctx.fr.f[n + 2];
    float v3 = ctx.fr.f[n + 3];

    ctx.fr.f[n + 0] = ctx.xf.f[0] * v0 + ctx.xf.f[4] * v1 + ctx.xf.f[8]  * v2 + ctx.xf.f[12] * v3;
    ctx.fr.f[n + 1] = ctx.xf.f[1] * v0 + ctx.xf.f[5] * v1 + ctx.xf.f[9]  * v2 + ctx.xf.f[13] * v3;
    ctx.fr.f[n + 2] = ctx.xf.f[2] * v0 + ctx.xf.f[6] * v1 + ctx.xf.f[10] * v2 + ctx.xf.f[14] * v3;
    ctx.fr.f[n + 3] = ctx.xf.f[3] * v0 + ctx.xf.f[7] * v1 + ctx.xf.f[11] * v2 + ctx.xf.f[15] * v3;
#endif
}

} // namespace dc::runtime::ops
