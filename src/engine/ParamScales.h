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

#ifndef OBXF_SRC_ENGINE_PARAMSCALES_H
#define OBXF_SRC_ENGINE_PARAMSCALES_H

#include <cmath>

/*
 * Normalized (0..1) to native unit scaling. Kept in a leaf header so the
 * modulation matrix can name the same curves the parameters use without
 * pulling in the rest of Utils.h.
 */

inline static float linsc(float param, const float min, const float max)
{
    return (param) * (max - min) + min;
}

inline static float logsc(float param, const float min, const float max, const float rolloff = 19.f)
{
    return ((std::exp(param * std::log(rolloff + 1.f)) - 1.f) / (rolloff)) * (max - min) + min;
}

#endif // OBXF_SRC_ENGINE_PARAMSCALES_H
