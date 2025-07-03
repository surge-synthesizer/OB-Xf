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

#ifndef OBXF_SRC_ENGINE_FILTER_H
#define OBXF_SRC_ENGINE_FILTER_H

#include "Voice.h"
#include <math.h>

class Filter
{
  private:
    float s1, s2, s3, s4;
    float R12, R24;
    float rcor12, rcorInv12;
    float rcor24, rcorInv24;

    float multimodeXfade;
    int multimodePole;

  public:
    float sampleRate;
    float sampleRateInv;
    bool bandPassSw;
    float multimode;
    bool selfOscPush;
    bool xpander;
    uint8_t xpanderMode;

    Filter()
    {
        selfOscPush = false;
        xpander = false;
        xpanderMode = 0;
        bandPassSw = false;
        multimode = 0.f;
        s1 = s2 = s3 = s4 = 0.f;
        sampleRate = 44000.f;
        sampleRateInv = 1.f / sampleRate;
        rcor12 = 500.f / 44000.f;
        rcorInv12 = 1.f / rcor12;
        rcor24 = 970.f / 44000.f;
        rcorInv24 = 1.f / rcor24;
        R12 = 1.f;
        R24 = 0.f;
    }

    void setMultimode(float m)
    {
        multimode = m;
        multimodePole = (int)(multimode * 3);
        multimodeXfade = multimode * 3 - multimodePole;
    }

    inline void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.f / sampleRate;

        float rcrate = sqrt((44000.f / sampleRate));

        rcor12 = (500.f / 44000.f) * rcrate;
        rcor24 = (970.f / 44000.f) * rcrate;
        rcorInv12 = 1.f / rcor12;
        rcorInv24 = 1.f / rcor24;
    }

    inline void setResonance(float res)
    {
        R12 = 1.f - res;
        R24 = (3.5f * res);
    }

    inline float diodePairResistanceApprox(float x)
    {
        // Taylor approximation of a slightly mismatched diode pair
        return (((((0.0103592f) * x + 0.00920833f) * x + 0.185f) * x + 0.05f) * x + 1.f);
    }

    inline float resolveFeedback2Pole(float sample, float g)
    {
        // calculating feedback non-linear transconducance and compensated for R12(-1)
        float tCfb;

        // boosting non-linearity
        float push = -1.f - (selfOscPush * 0.035f);

        tCfb = diodePairResistanceApprox(s1 * 0.0876f) + push;

        // disable non-linearity (digital filter)
        // tCfb = 0;

        // resolve linear feedback
        float y = ((sample - 2.f * (s1 * (R12 + tCfb)) - g * s1 - s2) /
                   (1.f + g * (2.f * (R12 + tCfb) + g)));

        return y;
    }

    inline float apply2Pole(float sample, float g)
    {
        float gpw = tanf(g * sampleRateInv * pi);

        g = gpw;

        float v = resolveFeedback2Pole(sample, g);

        float y1 = v * g + s1;
        s1 = v * g + y1;

        float y2 = y1 * g + s2;
        s2 = y1 * g + y2;

        float out;

        if (!bandPassSw)
        {
            out = (1.f - multimode) * y2 + (multimode * v);
        }
        else
        {
            out = 2.f * (multimode < 0.5f ? ((0.5f - multimode) * y2 + (multimode * y1))
                                          : ((1.f - multimode) * y1 + (multimode - 0.5f) * v));
        }

        return out;
    }

    inline float resolveFeedback4Pole(float sample, float g, float lpc)
    {
        float ml = 1.f / (1.f + g);
        float S = (lpc * (lpc * (lpc * s1 + s2) + s3) + s4) * ml;
        float G = lpc * lpc * lpc * lpc;
        float y = (sample - R24 * S) / (1.f + R24 * G);

        return y;
    }

    inline float apply4Pole(float sample, float g)
    {
        float g1 = (float)tan(g * sampleRateInv * pi);
        g = g1;

        float lpc = g / (1.f + g);
        float y0 = resolveFeedback4Pole(sample, g, lpc);

        // first lowpass in the cascade
        double v = (y0 - s1) * lpc;
        double res = v + s1;

        s1 = res + v;

        // damping
        s1 = atan(s1 * rcor24) * rcorInv24;

        float y1 = res;
        float y2 = tpt_process(s2, y1, g);
        float y3 = tpt_process(s3, y2, g);
        float y4 = tpt_process(s4, y3, g);
        float out;

        if (xpander)
        {
            // TODO Pole mixing!
            out = 0.f;
        }
        else
        {
            switch (multimodePole)
            {
            case 0:
                out = ((1.f - multimodeXfade) * y4 + (multimodeXfade)*y3);
                break;
            case 1:
                out = ((1.f - multimodeXfade) * y3 + (multimodeXfade)*y2);
                break;
            case 2:
                out = ((1.f - multimodeXfade) * y2 + (multimodeXfade)*y1);
                break;
            case 3:
                out = y1;
                break;
            default:
                out = 0.f;
                break;
            }
        }

        // half volume compensation
        return out * (1.f + R24 * 0.45f);
    }
};

#endif // OBXF_SRC_ENGINE_FILTER_H
