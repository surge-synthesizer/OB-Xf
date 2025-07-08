/*
 * OB-Xf - a continuation of the last open source version of OB-Xd.
 *
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more info,
 * see "HISTORY.md" in the root of this repository.
 *
 * This repository is a successor to the last open source release,
 * a version marked as 2.11. Copyright 2013-2025 by the authors
 * as indicated in the original release, and subsequent authors
 * per the GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_PULSEOSC_H
#define OBXF_SRC_ENGINE_PULSEOSC_H

#include "SynthEngine.h"
#include "BlepData.h"

class PulseOsc
{
    DelayLine<B_SAMPLES, float> del1;
    bool pw1t;
    float buffer1[B_SAMPLES * 2];
    const int n;
    float const *blepPTR;
    int bP1;

  public:
    PulseOsc() : n(B_SAMPLES * 2)
    {
        pw1t = false;
        bP1 = 0;
        for (int i = 0; i < n; i++)
            buffer1[i] = 0;
        blepPTR = blep;
    }

    ~PulseOsc() {}

    inline void setDecimation() { blepPTR = blepd2; }

    inline void removeDecimation() { blepPTR = blep; }

    inline float aliasReduction() { return -getNextBlep(buffer1, bP1); }

    inline void processLeader(float x, float delta, float pulseWidth, float pulseWidthWas)
    {
        float summated = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (pw1t)
                mixInImpulseCenter(buffer1, bP1, x / delta, 1);

            pw1t = false;
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - summated <= pulseWidth))
        {
            pw1t = true;
            float frac = (x - pulseWidth) / summated;
            mixInImpulseCenter(buffer1, bP1, frac, -1.f);
        }

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;
            if (pw1t)
                mixInImpulseCenter(buffer1, bP1, x / delta, 1.f);
            pw1t = false;
        }
    }

    inline float getValue(float x, float pulseWidth)
    {
        float oscmix;
        if (x >= pulseWidth)
            oscmix = 1.f - (0.5f - pulseWidth) - 0.5f;
        else
            oscmix = -(0.5f - pulseWidth) - 0.5f;
        return del1.feedReturn(oscmix);
    }

    inline float getValueFast(float x, float pulseWidth)
    {
        float oscmix;
        if (x >= pulseWidth)
            oscmix = 1.f - (0.5f - pulseWidth) - 0.5f;
        else
            oscmix = -(0.5f - pulseWidth) - 0.5f;
        return oscmix;
    }

    inline void processFollower(float x, float delta, bool hardSyncReset, float hardSyncFrac,
                                float pulseWidth, float pulseWidthWas)
    {
        float summated = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                if (pw1t)
                    mixInImpulseCenter(buffer1, bP1, x / delta, 1.f);
                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - summated <= pulseWidth))
        {
            pw1t = true;
            float frac = (x - pulseWidth) / summated;
            if (((!hardSyncReset) || (frac > hardSyncFrac))) // de morgan processed equation
            {
                // transition to 1
                mixInImpulseCenter(buffer1, bP1, frac, -1.f);
            }
            else
            {
                // if transition do not ocurred
                pw1t = false;
            }
        }
        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                if (pw1t)
                    mixInImpulseCenter(buffer1, bP1, x / delta, 1.f);
                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if (hardSyncReset)
        {
            // float fracMaster = (delta * hardSyncFrac);
            float trans = (pw1t ? 1.f : 0.f);
            mixInImpulseCenter(buffer1, bP1, hardSyncFrac, trans);
            pw1t = false;
        }
    }

    inline void mixInImpulseCenter(float *buf, int &bpos, float offset, float scale)
    {
        const float *table = blepPTR;
        const size_t tableSize = (table == blep) ? std::size(blep) : std::size(blepd2);

        int lpIn = static_cast<int>(B_OVERSAMPLING * (offset));
        const int maxIter = (static_cast<int>(tableSize) - 1 - lpIn) / B_OVERSAMPLING + 1;
        const int safeSamples = std::min(B_SAMPLES, maxIter);
        const int safeN = std::min(n, maxIter);

        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1 = 1.f - frac;
        for (int i = 0; i < safeSamples; i++)
        {
            const float mixValue = (table[lpIn] * f1 + table[lpIn + 1] * frac);
            buf[(bpos + i) & (n - 1)] += mixValue * scale;
            lpIn += B_OVERSAMPLING;
        }
        for (int i = safeSamples; i < safeN; i++)
        {
            const float mixValue = (table[lpIn] * f1 + table[lpIn + 1] * frac);
            buf[(bpos + i) & (n - 1)] -= mixValue * scale;
            lpIn += B_OVERSAMPLING;
        }
    }

    inline float getNextBlep(float *buf, int &bpos)
    {
        buf[bpos] = 0.f;
        bpos++;

        // Wrap pos
        bpos &= (n - 1);
        return buf[bpos];
    }
};

#endif // OBXF_SRC_ENGINE_PULSEOSC_H
