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

#ifndef OBXF_SRC_ENGINE_AUDIOUTILS_H
#define OBXF_SRC_ENGINE_AUDIOUTILS_H

#include <Utils.h>
#include "SynthEngine.h"

inline static float tpt_process_scaled_cutoff(float &state, float input,
                                              float cutoff_over_onepluscutoff)
{
    const float v = (input - state) * cutoff_over_onepluscutoff;
    const float res = v + state;

    // v + v is exact, so the state rounds once
    state = state + (v + v);

    return res;
}

// c / (1 + c) for an unwarped cutoff, which callers can keep instead of recomputing
inline static float tpt_unwarped_scale(float cutoff, float srInv)
{
    cutoff = (cutoff * srInv) * pi;
    return cutoff / (1.f + cutoff);
}

inline static float tpt_lp_unwarped(float &state, float input, float cutoff, float srInv)
{
    return tpt_process_scaled_cutoff(state, input, tpt_unwarped_scale(cutoff, srInv));
}

inline static float tpt_lp(float &state, float input, float cutoff, float srInv)
{
    cutoff = tan(cutoff * srInv * pi);

    return tpt_process_scaled_cutoff(state, input, cutoff / (1.f + cutoff));
};

inline static float tpt_process(float &state, float input, float cutoff)
{
    return tpt_process_scaled_cutoff(state, input, cutoff / (1.f + cutoff));
}

#endif // OBXF_SRC_ENGINE_AUDIOUTILS_H
