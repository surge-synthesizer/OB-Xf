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

#ifndef OBXF_SRC_ENGINE_GROUPROWS_H
#define OBXF_SRC_ENGINE_GROUPROWS_H

#include "Voice.h"

/*
 * What one group of four voices hands from stage to stage within a block, one
 * row of four lanes per sub-sample, so the SIMD stages never transpose between
 * them. The scalar fallbacks copy a lane out to the voice's own block arrays.
 */
struct alignas(16) GroupRows
{
    static constexpr int lanes{4};
    static constexpr int maxSubs{Voice::maxBlockSubSamples};

    using Row = float[maxSubs][lanes];

    // the control stage's inputs
    Row cutoffPar, bend, lfo1Mod, lfo2Mod, noise, envDelayed, lfo2, ampEnv;

    // the control stage's outputs, which the oscillators and filter read
    Row notePlaying, cutoff, osc1PWMod, osc2PWMod, osc1PitchMod, osc2PitchMod;

    // the oscillators' output, which the filter reads
    Row out;

    static void toVoice(const Row &row, int l, float *dst, int n)
    {
        for (int s = 0; s < n; ++s)
        {
            dst[s] = row[s][l];
        }
    }

    static void fromVoice(Row &row, int l, const float *src, int n)
    {
        for (int s = 0; s < n; ++s)
        {
            row[s][l] = src[s];
        }
    }
};

#endif // OBXF_SRC_ENGINE_GROUPROWS_H
