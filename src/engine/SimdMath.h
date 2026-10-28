/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_SIMDMATH_H
#define OBXF_SRC_ENGINE_SIMDMATH_H

#include <cstdint>
#include <cstring>

#include "sst/basic-blocks/simd/setup.h"

// arm64ec also defines the x86 intrinsics, and its __m128 isn't simde's
#if defined(SST_SIMD_ARM64) && !defined(SST_SIMD_NATIVE_X86)
#define OBXF_SIMD_FUSED_MULADD
#include "simde/x86/fma.h"
#endif

/*
 * Four-lane helpers for code that must match its scalar original bit for bit.
 * arm64 compilers fuse a * b + c inside one expression of the scalar code,
 * x86 builds without FMA do not, so the multiply-adds follow suit.
 */
namespace obxf_simd
{
inline SIMD_M128 mulAdd(SIMD_M128 a, SIMD_M128 b, SIMD_M128 c)
{
#ifdef OBXF_SIMD_FUSED_MULADD
    return simde_mm_fmadd_ps(a, b, c);
#else
    return SIMD_MM(add_ps)(SIMD_MM(mul_ps)(a, b), c);
#endif
}

// c - a * b
inline SIMD_M128 negMulAdd(SIMD_M128 a, SIMD_M128 b, SIMD_M128 c)
{
#ifdef OBXF_SIMD_FUSED_MULADD
    return simde_mm_fnmadd_ps(a, b, c);
#else
    return SIMD_MM(sub_ps)(c, SIMD_MM(mul_ps)(a, b));
#endif
}

inline SIMD_M128 blend(SIMD_M128 a, SIMD_M128 b, SIMD_M128 mask)
{
    return SIMD_MM(or_ps)(SIMD_MM(andnot_ps)(mask, a), SIMD_MM(and_ps)(mask, b));
}

inline float allBits()
{
    const uint32_t bits{0xFFFFFFFFu};
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

/*
 * 2^x from exponent bits and a degree 7 taylor series on [-0.5, 0.5], within
 * float precision. Resonant patches magnify any pitch error, so it pays to be.
 */
inline SIMD_M128 exp2(SIMD_M128 x)
{
    x = SIMD_MM(max_ps)(x, SIMD_MM(set1_ps)(-126.f));

    const auto whole = SIMD_MM(cvtps_epi32)(x);
    const auto f = SIMD_MM(sub_ps)(x, SIMD_MM(cvtepi32_ps)(whole));

    auto p = SIMD_MM(set1_ps)(1.5252734e-5f);
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(1.5403530e-4f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(1.3333558e-3f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(0.00961813f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(0.05550411f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(0.24022650f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(0.69314718f), SIMD_MM(mul_ps)(f, p));
    p = SIMD_MM(add_ps)(SIMD_MM(set1_ps)(1.f), SIMD_MM(mul_ps)(f, p));

    return SIMD_MM(castsi128_ps)(
        SIMD_MM(add_epi32)(SIMD_MM(castps_si128)(p), SIMD_MM(slli_epi32)(whole, 23)));
}
} // namespace obxf_simd

#endif // OBXF_SRC_ENGINE_SIMDMATH_H
