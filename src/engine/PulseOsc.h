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
        /* when x >= pw: 1-(0.5-pw)-0.5 = pw; when x < pw: -(0.5-pw)-0.5 = pw-1 */
        const float oscmix = pulseWidth - 1.f + static_cast<float>(x >= pulseWidth);
        return delay.feedReturn(oscmix);
    }

    inline float getValueFast(float x, float pulseWidth)
    {
        return pulseWidth - 1.f + static_cast<float>(x >= pulseWidth);
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
        int lpIn = static_cast<int>(B_OVERSAMPLING * offset);
        if (lpIn >= B_OVERSAMPLING)
            lpIn = B_OVERSAMPLING - 1;

        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1s = (1.f - frac) * scale;
        const float fracs = frac * scale;

        const float *rowA = blepPtr + lpIn * B_SAMPLESx2;
        const float *rowB = rowA + B_SAMPLESx2;

        for (int i = 0; i < B_SAMPLES; i++)
            buf[(bpos + i) & (B_SAMPLESx2 - 1)] += rowA[i] * f1s + rowB[i] * fracs;

        for (int i = B_SAMPLES; i < B_SAMPLESx2; i++)
            buf[(bpos + i) & (B_SAMPLESx2 - 1)] -= rowA[i] * f1s + rowB[i] * fracs;
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