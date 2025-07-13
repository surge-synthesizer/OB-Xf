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
    DelayLine<B_SAMPLES, float> delay;

    float buffer[B_SAMPLESx2]{0.f};
    const float *blepPtr{blep};

    bool pw1t{false};
    int bufferPos{0};

  public:
    PulseOsc() = default;
    ~PulseOsc() = default;

    inline void setDecimation() { blepPtr = blepd2; }

    inline void removeDecimation() { blepPtr = blep; }

    inline float aliasReduction() { return -getNextBlep(buffer, bufferPos); }

    inline void processLeader(float x, float delta, float pulseWidth, float pulseWidthWas)
    {
        float sum = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (pw1t)
            {
                mixInImpulseCenter(buffer, bufferPos, x / delta, 1);
            }

            pw1t = false;
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - sum <= pulseWidth))
        {
            float frac = (x - pulseWidth) / sum;

            pw1t = true;

            mixInImpulseCenter(buffer, bufferPos, frac, -1.f);
        }

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (pw1t)
            {
                mixInImpulseCenter(buffer, bufferPos, x / delta, 1.f);
            }

            pw1t = false;
        }
    }

    inline float getValue(float x, float pulseWidth)
    {
        float oscmix;

        if (x >= pulseWidth)
        {
            oscmix = 1.f - (0.5f - pulseWidth) - 0.5f;
        }
        else
        {
            oscmix = -(0.5f - pulseWidth) - 0.5f;
        }

        return delay.feedReturn(oscmix);
    }

    inline float getValueFast(float x, float pulseWidth)
    {
        float oscmix;

        if (x >= pulseWidth)
        {
            oscmix = 1.f - (0.5f - pulseWidth) - 0.5f;
        }
        else
        {
            oscmix = -(0.5f - pulseWidth) - 0.5f;
        }

        return oscmix;
    }

    inline void processFollower(float x, float delta, bool hardSyncReset, float hardSyncFrac,
                                float pulseWidth, float pulseWidthWas)
    {
        float sum = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            // De Morgan processed equation
            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                if (pw1t)
                {
                    mixInImpulseCenter(buffer, bufferPos, x / delta, 1.f);
                }

                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - sum <= pulseWidth))
        {
            float frac = (x - pulseWidth) / sum;

            pw1t = true;

            // De Morgan processed equation
            if (((!hardSyncReset) || (frac > hardSyncFrac)))
            {
                // transition to 1
                mixInImpulseCenter(buffer, bufferPos, frac, -1.f);
            }
            else
            {
                // if transition didn't ocurr
                pw1t = false;
            }
        }
        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            // De Morgan processed equation
            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                if (pw1t)
                {
                    mixInImpulseCenter(buffer, bufferPos, x / delta, 1.f);
                }

                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if (hardSyncReset)
        {
            float trans = (pw1t ? 1.f : 0.f);

            mixInImpulseCenter(buffer, bufferPos, hardSyncFrac, trans);

            pw1t = false;
        }
    }

    inline void mixInImpulseCenter(float *buf, int &bpos, float offset, float scale)
    {
        const float *table = blepPtr;
        const size_t tableSize = (table == blep) ? std::size(blep) : std::size(blepd2);

        int lpIn = static_cast<int>(B_OVERSAMPLING * (offset));
        const int maxIter = (static_cast<int>(tableSize) - 1 - (lpIn +1)) / B_OVERSAMPLING + 1;
        const int safeSamples = std::min(B_SAMPLES, maxIter);
        const int safeN = std::min(B_SAMPLESx2, maxIter);
        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1 = 1.f - frac;

        for (int i = 0; i < safeSamples; i++)
        {
            assert(static_cast<size_t>(lpIn) + 1 < tableSize );
            const float mixValue = (table[lpIn] * f1 + table[lpIn + 1] * frac);

            buf[(bpos + i) & (B_SAMPLESx2 - 1)] += mixValue * scale;
            lpIn += B_OVERSAMPLING;
        }

        for (int i = safeSamples; i < safeN; i++)
        {
            assert(static_cast<size_t>(lpIn) + 1 < tableSize );
            const float mixValue = (table[lpIn] * f1 + table[lpIn + 1] * frac);
            buf[(bpos + i) & (B_SAMPLESx2 - 1)] -= mixValue * scale;
            lpIn += B_OVERSAMPLING;
        }
    }

    inline float getNextBlep(float *buf, int &bpos)
    {
        buf[bpos] = 0.f;
        bpos++;

        // wrap position
        bpos &= (B_SAMPLESx2 - 1);

        return buf[bpos];
    }
};

#endif // OBXF_SRC_ENGINE_PULSEOSC_H