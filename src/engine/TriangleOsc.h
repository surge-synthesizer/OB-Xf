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

#ifndef OBXF_SRC_ENGINE_TRIANGLEOSC_H
#define OBXF_SRC_ENGINE_TRIANGLEOSC_H

#include "SynthEngine.h"
#include "BlepData.h"

class TriangleOsc
{
    DelayLine<B_SAMPLES, float> del1;
    bool fall;
    float buffer1[B_SAMPLES * 2];
    // const int hsam;
    const int n;
    float const *blepPTR;
    float const *blampPTR;

    int bP1, bP2;

  public:
    TriangleOsc() : n(B_SAMPLES * 2)
    {
        fall = false;
        bP1 = bP2 = 0;
        for (int i = 0; i < n; i++)
            buffer1[i] = 0;
        blepPTR = blep;
        blampPTR = blamp;
    }

    ~TriangleOsc() {}

    inline void setDecimation()
    {
        blepPTR = blepd2;
        blampPTR = blampd2;
    }

    inline void removeDecimation()
    {
        blepPTR = blep;
        blampPTR = blamp;
    }

    inline float aliasReduction() { return -getNextBlep(buffer1, bP1); }

    inline void processLeader(float x, float delta)
    {
        if (x >= 1.0)
        {
            x -= 1.0;
            mixInBlampCenter(buffer1, bP1, x / delta, -4 * B_SAMPLES * delta);
        }
        if (x >= 0.5 && x - delta < 0.5)
        {
            mixInBlampCenter(buffer1, bP1, (x - 0.5) / delta, 4 * B_SAMPLES * delta);
        }
        if (x >= 1.0)
        {
            x -= 1.0;
            mixInBlampCenter(buffer1, bP1, x / delta, -4 * B_SAMPLES * delta);
        }
    }

    inline float getValue(float x)
    {
        float mix = x < 0.5 ? 2 * x - 0.5 : 1.5 - 2 * x;
        return del1.feedReturn(mix);
    }

    inline float getValueFast(float x)
    {
        float mix = x < 0.5 ? 2 * x - 0.5 : 1.5 - 2 * x;
        return mix;
    }

    inline void processFollower(float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        bool hspass = true;
        if (x >= 1.0)
        {
            x -= 1.0;
            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                mixInBlampCenter(buffer1, bP1, x / delta, -4 * B_SAMPLES * delta);
            }
            else
            {
                x += 1;
                hspass = false;
            }
        }
        if (x >= 0.5 && x - delta < 0.5 && hspass)
        {
            float frac = (x - 0.5) / delta;
            if (((!hardSyncReset) || (frac > hardSyncFrac))) // de morgan processed equation
            {
                mixInBlampCenter(buffer1, bP1, frac, 4 * B_SAMPLES * delta);
            }
        }
        if (x >= 1.0 && hspass)
        {
            x -= 1.0;
            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                mixInBlampCenter(buffer1, bP1, x / delta, -4 * B_SAMPLES * delta);
            }
            else
            {
                // if transition do not ocurred
                x += 1;
            }
        }
        if (hardSyncReset)
        {
            float fracMaster = (delta * hardSyncFrac);
            float trans = (x - fracMaster);
            float mix = trans < 0.5 ? 2 * trans - 0.5 : 1.5 - 2 * trans;
            if (trans > 0.5)
                mixInBlampCenter(buffer1, bP1, hardSyncFrac, -4 * B_SAMPLES * delta);
            mixInImpulseCenter(buffer1, bP1, hardSyncFrac, mix + 0.5);
        }
    }

    inline void mixInBlampCenter(float *buf, int &bpos, float offset, float scale)
    {
        const float *table = blampPTR;
        const size_t tableSize = (table == blamp) ? std::size(blamp) : std::size(blampd2);

        int lpIn = static_cast<int>(B_OVERSAMPLING * (offset));
        int maxIter = (static_cast<int>(tableSize) - 1 - lpIn) / B_OVERSAMPLING + 1;
        const int safeN = std::min(n, maxIter);

        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1 = 1.0f - frac;
        for (int i = 0; i < safeN; i++)
        {
            const float mixValue = (table[lpIn] * f1 + table[lpIn + 1] * frac);
            buf[(bpos + i) & (n - 1)] += mixValue * scale;
            lpIn += B_OVERSAMPLING;
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

        const float frac = offset * B_OVERSAMPLING - lpIn;
        const float f1 = 1.0f - frac;
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
        buf[bpos] = 0.0f;
        bpos++;

        // Wrap pos
        bpos &= (n - 1);
        return buf[bpos];
    }
};

#endif // OBXF_SRC_ENGINE_TRIANGLEOSC_H
