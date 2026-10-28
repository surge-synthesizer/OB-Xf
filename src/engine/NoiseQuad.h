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

#ifndef OBXF_SRC_ENGINE_NOISEQUAD_H
#define OBXF_SRC_ENGINE_NOISEQUAD_H

#include <array>
#include <cstdint>

#include "Noise.h"
#include "SimdMath.h"

// neon's 32-bit multiply, through simde
#include "simde/x86/sse4.1.h"

/*
 * Noise's white and red generators for four voices at once, one generator per
 * lane, each stepping exactly as its own Noise would. Lanes outside the active
 * mask hold still.
 */
struct NoiseQuad
{
    static constexpr int lanes{4};

    void load(Noise *const n[lanes], int count)
    {
        alignas(16) int32_t states[lanes]{};
        alignas(16) float volComps[lanes]{}, reds[lanes]{};

        for (int l = 0; l < count; ++l)
        {
            states[l] = n[l]->white.state;
            volComps[l] = n[l]->white.volComp;
            reds[l] = n[l]->red.state;
        }

        state = SIMD_MM(load_si128)(reinterpret_cast<const SIMD_M128I *>(states));
        volComp = SIMD_MM(load_ps)(volComps);
        red = SIMD_MM(load_ps)(reds);
    }

    void store(Noise *const n[lanes], int count) const
    {
        alignas(16) int32_t states[lanes];
        alignas(16) float reds[lanes];

        SIMD_MM(store_si128)(reinterpret_cast<SIMD_M128I *>(states), state);
        SIMD_MM(store_ps)(reds, red);

        for (int l = 0; l < count; ++l)
        {
            n[l]->white.state = states[l];
            n[l]->red.state = reds[l];
        }
    }

    // Noise::getWhite
    SIMD_M128 white(SIMD_M128 active)
    {
        const auto next = step<1>(state);
        hold(next, active);
        return toWhite(next);
    }

    // three Noise::getWhite in a row, each jumped to from the same state so they overlap
    void whiteThree(SIMD_M128 active, SIMD_M128 &a, SIMD_M128 &b, SIMD_M128 &c)
    {
        const auto last = step<3>(state);
        a = toWhite(step<1>(state));
        b = toWhite(step<2>(state));
        c = toWhite(last);
        hold(last, active);
    }

    // Noise::getRed, on a white sample already drawn
    SIMD_M128 redFrom(SIMD_M128 w, SIMD_M128 active)
    {
        using namespace obxf_simd;

        auto r = mulAdd(w, SIMD_MM(set1_ps)(0.05f), red);

        const auto high = SIMD_MM(cmpgt_ps)(r, SIMD_MM(set1_ps)(1.f));
        const auto low = SIMD_MM(andnot_ps)(high, SIMD_MM(cmplt_ps)(r, SIMD_MM(set1_ps)(-1.f)));

        r = blend(r, SIMD_MM(sub_ps)(SIMD_MM(set1_ps)(2.f), r), high);
        r = blend(r, SIMD_MM(sub_ps)(SIMD_MM(set1_ps)(-2.f), r), low);

        red = blend(red, r, active);

        return r;
    }

  private:
    // the generator's n-th next state, as the scalar's unsigned arithmetic wraps
    template <int n> static SIMD_M128I step(SIMD_M128I s)
    {
        constexpr auto jump = [] {
            uint32_t mul = 1, add = 0;

            for (int i = 0; i < n; ++i)
            {
                mul *= 1103515245u;
                add = add * 1103515245u + 12345u;
            }

            return std::array<uint32_t, 2>{mul, add};
        }();

        return SIMD_MM(add_epi32)(mullo(s, SIMD_MM(set1_epi32)(int32_t(jump[0]))),
                                  SIMD_MM(set1_epi32)(int32_t(jump[1])));
    }

    // the low 32 bits of each lane's product
    static SIMD_M128I mullo(SIMD_M128I a, SIMD_M128I b)
    {
#ifdef SST_SIMD_NATIVE_X86
        // sse2 has no 32-bit multiply, and arm64ec's __m128i isn't simde's
        const auto even = SIMD_MM(mul_epu32)(a, b);
        const auto odd = SIMD_MM(mul_epu32)(SIMD_MM(srli_epi64)(a, 32), SIMD_MM(srli_epi64)(b, 32));
        return SIMD_MM(unpacklo_epi32)(SIMD_MM(shuffle_epi32)(even, SIMD_MM_SHUFFLE(0, 0, 2, 0)),
                                       SIMD_MM(shuffle_epi32)(odd, SIMD_MM_SHUFFLE(0, 0, 2, 0)));
#else
        return simde_mm_mullo_epi32(a, b);
#endif
    }

    void hold(SIMD_M128I next, SIMD_M128 active)
    {
        const auto mask = SIMD_MM(castps_si128)(active);
        state =
            SIMD_MM(or_si128)(SIMD_MM(andnot_si128)(mask, state), SIMD_MM(and_si128)(mask, next));
    }

    SIMD_M128 toWhite(SIMD_M128I s) const
    {
        return SIMD_MM(mul_ps)(SIMD_MM(cvtepi32_ps)(s), volComp);
    }

    SIMD_M128I state;
    SIMD_M128 volComp, red;
};

#endif // OBXF_SRC_ENGINE_NOISEQUAD_H
