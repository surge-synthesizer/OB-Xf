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
    DelayLine<Samples> del1;
    bool pw1t;
    float buffer1[Samples * 2];
    // const int hsam;
    const int n;
    float const *blepPTR;
    int bP1;

  public:
    PulseOsc()
        : // hsam(Samples),
          n(Samples * 2)
    {
        //	del1 = new DelayLine(hsam);
        pw1t = false;
        bP1 = 0;
        // buffer1= new float[n];
        for (int i = 0; i < n; i++)
            buffer1[i] = 0;
        blepPTR = blep;
    }
    ~PulseOsc()
    {
        //	delete buffer1;
        //	delete del1;
    }
    inline void setDecimation() { blepPTR = blepd2; }
    inline void removeDecimation() { blepPTR = blep; }
    inline float aliasReduction() { return -getNextBlep(buffer1, bP1); }
    inline void processMaster(float x, float delta, float pulseWidth, float pulseWidthWas)
    {
        float summated = delta - (pulseWidth - pulseWidthWas);
        if ((pw1t) && x >= 1.0f)
        {
            x -= 1.0f;
            if (pw1t)
                mixInImpulseCenter(buffer1, bP1, x / delta, 1);
            pw1t = false;
        }
        if ((!pw1t) && (x >= pulseWidth) && (x - summated <= pulseWidth))
        {
            pw1t = true;
            float frac = (x - pulseWidth) / summated;
            mixInImpulseCenter(buffer1, bP1, frac, -1);
        }
        if ((pw1t) && x >= 1.0f)
        {
            x -= 1.0f;
            if (pw1t)
                mixInImpulseCenter(buffer1, bP1, x / delta, 1);
            pw1t = false;
        }
    }
    inline float getValue(float x, float pulseWidth)
    {
        float oscmix;
        if (x >= pulseWidth)
            oscmix = 1 - (0.5 - pulseWidth) - 0.5;
        else
            oscmix = -(0.5 - pulseWidth) - 0.5;
        return del1.feedReturn(oscmix);
    }
    inline float getValueFast(float x, float pulseWidth)
    {
        float oscmix;
        if (x >= pulseWidth)
            oscmix = 1 - (0.5 - pulseWidth) - 0.5;
        else
            oscmix = -(0.5 - pulseWidth) - 0.5;
        return oscmix;
    }
    inline void processSlave(float x, float delta, bool hardSyncReset, float hardSyncFrac,
                             float pulseWidth, float pulseWidthWas)
    {
        float summated = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.0f)
        {
            x -= 1.0f;
            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                if (pw1t)
                    mixInImpulseCenter(buffer1, bP1, x / delta, 1);
                pw1t = false;
            }
            else
            {
                x += 1;
            }
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - summated <= pulseWidth))
        {
            pw1t = true;
            float frac = (x - pulseWidth) / summated;
            if (((!hardSyncReset) || (frac > hardSyncFrac))) // de morgan processed equation
            {
                // transition to 1
                mixInImpulseCenter(buffer1, bP1, frac, -1);
            }
            else
            {
                // if transition do not ocurred
                pw1t = false;
            }
        }
        if ((pw1t) && x >= 1.0f)
        {
            x -= 1.0f;
            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                if (pw1t)
                    mixInImpulseCenter(buffer1, bP1, x / delta, 1);
                pw1t = false;
            }
            else
            {
                x += 1;
            }
        }

        if (hardSyncReset)
        {
            // float fracMaster = (delta * hardSyncFrac);
            float trans = (pw1t ? 1 : 0);
            mixInImpulseCenter(buffer1, bP1, hardSyncFrac, trans);
            pw1t = false;
        }
    }
    inline void mixInImpulseCenter(float *buf, int &bpos, float offset, float scale)
    {
        int lpIn = (int)(B_OVERSAMPLING * (offset));
        float frac = offset * B_OVERSAMPLING - lpIn;
        float f1 = 1.0f - frac;
        for (int i = 0; i < Samples; i++)
        {
            float mixvalue = (blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * (frac));
            buf[(bpos + i) & (n - 1)] += mixvalue * scale;
            lpIn += B_OVERSAMPLING;
        }
        for (int i = Samples; i < n; i++)
        {
            float mixvalue = (blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * (frac));
            buf[(bpos + i) & (n - 1)] -= mixvalue * scale;
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

#endif // OBXF_SRC_ENGINE_PULSEOSC_H
