/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_SAWOSC_H
#define OBXF_SRC_ENGINE_SAWOSC_H
#include "SynthEngine.h"
#include "BlepData.h"
class SawOsc
{
    DelayLine<Samples> del1;
    float buffer1[Samples * 2];
    const int n;
    float const *blepPTR;
    int bP1;

  public:
    SawOsc()
        : // hsam(Samples),
          n(Samples * 2)
    {
        bP1 = 0;
        // del1 = new DelayLine(hsam);
        // buffer1= new float[n];
        for (int i = 0; i < n; i++)
            buffer1[i] = 0;
        blepPTR = blep;
    }
    ~SawOsc()
    {
        // delete del1;
        // delete buffer1;
    }
    inline void setDecimation() { blepPTR = blepd2; }
    inline void removeDecimation() { blepPTR = blep; }
    inline float aliasReduction() { return -getNextBlep(buffer1, bP1); }
    inline void processMaster(float x, float delta)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;
            mixInImpulseCenter(buffer1, bP1, x / delta, 1);
        }
    }
    inline float getValue(float x) { return del1.feedReturn(x - 0.5); }
    inline float getValueFast(float x) { return x - 0.5; }
    inline void processSlave(float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;
            if (((!hardSyncReset) || (x / delta > hardSyncFrac))) // de morgan processed equation
            {
                mixInImpulseCenter(buffer1, bP1, x / delta, 1);
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
            mixInImpulseCenter(buffer1, bP1, hardSyncFrac, trans);
        }
    }
    inline void mixInImpulseCenter(float *buf, int &bpos, float offset, float scale)
    {
        int lpIn = (int)(B_OVERSAMPLING * (offset));
        float frac = offset * B_OVERSAMPLING - lpIn;
        float f1 = 1.0f - frac;
        for (int i = 0; i < Samples; i++)
        {
            float mixvalue = (blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * frac);
            buf[(bpos + i) & (n - 1)] += mixvalue * scale;
            lpIn += B_OVERSAMPLING;
        }
        for (int i = Samples; i < n; i++)
        {
            float mixvalue = (blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * frac);
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

#endif // OBXF_SRC_ENGINE_SAWOSC_H
