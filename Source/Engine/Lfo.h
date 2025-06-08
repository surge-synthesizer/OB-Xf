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

#ifndef OBXF_SRC_ENGINE_LFO_H
#define OBXF_SRC_ENGINE_LFO_H
#include "SynthEngine.h"
class Lfo
{
  private:
    float phase;
    float s, sq, sh;
    float s1;
    juce::Random rg;
    float SampleRate;
    float SampleRateInv;

    float syncRate;
    bool synced;

  public:
    float Frequency;
    float phaseInc;
    float frUnsc; // frequency value without sync
    float rawParam;
    int waveForm;
    Lfo()
    {
        phaseInc = 0;
        frUnsc = 0;
        syncRate = 1;
        rawParam = 0;
        synced = false;
        s1 = 0;
        Frequency = 1;
        phase = 0;
        s = sq = sh = 0;
        rg = juce::Random();
    }
    void setSynced()
    {
        synced = true;
        recalcRate(rawParam);
    }
    void setUnsynced()
    {
        synced = false;
        phaseInc = frUnsc;
    }
    void hostSyncRetrigger(float bpm, float quaters)
    {
        if (synced)
        {
            phaseInc = (bpm / 60.0) * syncRate;
            phase = phaseInc * quaters;
            phase = (fmod(phase, 1) * juce::MathConstants<float>::pi * 2 -
                     juce::MathConstants<float>::pi);
        }
    }
    inline float getVal()
    {
        float Res = 0;
        if ((waveForm & 1) != 0)
            Res += s;
        if ((waveForm & 2) != 0)
            Res += sq;
        if ((waveForm & 4) != 0)
            Res += sh;
        return tptlpupw(s1, Res, 3000, SampleRateInv);
    }
    void setSamlpeRate(float sr)
    {
        SampleRate = sr;
        SampleRateInv = 1 / SampleRate;
    }
    inline void update()
    {
        phase += ((phaseInc * juce::MathConstants<float>::pi * 2 * SampleRateInv));
        sq = (phase > 0 ? 1 : -1);
        s = sin(phase);
        if (phase > juce::MathConstants<float>::pi)
        {
            phase -= 2 * juce::MathConstants<float>::pi;
            sh = rg.nextFloat() * 2 - 1;
        }
    }
    void setFrequency(float val)
    {
        frUnsc = val;
        if (!synced)
            phaseInc = val;
    }
    void setRawParam(float param) // used for synced rate changes
    {
        rawParam = param;
        if (synced)
        {
            recalcRate(param);
        }
    }
    void recalcRate(float param)
    {
        const int ratesCount = 9;
        int parval = (int)(param * (ratesCount - 1));
        float rt = 1;
        switch (parval)
        {
        case 0:
            rt = 1.0 / 8;
            break;
        case 1:
            rt = 1.0 / 4;
            break;
        case 2:
            rt = 1.0 / 3;
            break;
        case 3:
            rt = 1.0 / 2;
            break;
        case 4:
            rt = 1.0;
            break;
        case 5:
            rt = 3.0 / 2;
            break;
        case 6:
            rt = 2;
            break;
        case 7:
            rt = 3;
            break;
        case 8:
            rt = 4;
            break;
        default:
            rt = 1;
            break;
        }
        syncRate = rt;
    }
};

#endif // OBXF_SRC_ENGINE_LFO_H
