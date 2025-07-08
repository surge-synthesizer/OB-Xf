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

#ifndef OBXF_SRC_ENGINE_LFO_H
#define OBXF_SRC_ENGINE_LFO_H

#include <Constants.h>
#include <juce_dsp/juce_dsp.h>
#include "SynthEngine.h"

class LFO
{
  private:
    float phase{0.f};
    float sine{0.f}, square{0.f}, saw{0.f}, tri{0.f}, samplehold{0.f}, sampleglide{0.f},
        sg_history{0.f};
    float sum{0.f};
    juce::Random rnd;

    float sampleRate{1.f};
    float sampleRateInv{1.f};

    bool synced{false};
    float syncRate{1.f};
    float unsyncedRate{0.f};

    float rawParam{0.f};

  public:
    float phaseInc{0.f};

    float pw{0.f};

    float wave1blend{0.f};
    float wave2blend{0.f};
    float wave3blend{0.f};

    bool unipolarPulse{false};

    LFO()
    {
        rnd = juce::Random();
        samplehold = rnd.nextFloat() * 2.f - 1.f;
        sg_history = samplehold;
    }

    void setSynced()
    {
        synced = true;
        recalcRate(rawParam);
    }

    void setUnsynced()
    {
        synced = false;
        phaseInc = unsyncedRate;
    }

    void hostSyncRetrigger(float bpm, float quaters)
    {
        if (synced)
        {
            phaseInc = (bpm / 60.f) * syncRate;
            phase = phaseInc * quaters;
            phase = fmod(phase, 1.f) * twoPi - pi;
        }
    }

    inline float getVal()
    {
        float result = 0.f;

        if (wave1blend >= 0.f)
            result += tri * wave1blend;
        else
            result += sine * -wave1blend;

        if (wave2blend >= 0.f)
            result += saw * wave2blend;
        else
            result += square * -wave2blend;

        if (wave3blend >= 0.f)
            result += sampleglide * wave3blend;
        else
            result += samplehold * -wave3blend;

        return tpt_lp_unwarped(sum, result, 3000.f, sampleRateInv);
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.f / sr;
    }

    inline float bend(float x, float d) const
    {
        if (d == 0)
        {
            return x;
        }

        auto a = 0.5 * d;

        x = x - a * x * x + a;
        x = x - a * x * x + a;
        x = x - a * x * x + a;

        return x;
    }

    inline void update()
    {
        phase += ((phaseInc * twoPi * sampleRateInv));

        if (phase > pi)
        {
            phase -= twoPi;
            sg_history = samplehold;
            samplehold = rnd.nextFloat() * 2.f - 1.f;
        }

        // casting dance is to satisfy MSVC Clang
        sine = static_cast<float>(juce::dsp::FastMathApproximations::sin<double>(phase));
        tri = (twoByPi * abs(phase + halfPi - (phase > halfPi) * twoPi)) - 1.f;
        square = (phase > (pi * pw * 0.9f) ? 1.f : -1.f + unipolarPulse);
        saw = bend(-phase * invPi, -pw);
        sampleglide = sg_history + (samplehold - sg_history) * (pi + phase) * invTwoPi;
    }

    // valid input value range is 0...1, otherwise phase will not be set!
    void setPhaseDirectly(float val)
    {
        if (val >= 0.f && val >= 1.f)
        {
            phase = (val * twoPi) - pi;
        }
    }

    void setFrequency(float val)
    {
        unsyncedRate = val;

        if (!synced)
        {
            phaseInc = val;
        }
    }

    // used for synced rate changes
    void setRawParam(float param)
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
        float rt = 1.f;

        switch (parval)
        {
        case 0:
            rt = 1.f / 8.f;
            break;
        case 1:
            rt = 1.f / 4.f;
            break;
        case 2:
            rt = 0.3f;
            break;
        case 3:
            rt = 1.f / 2.f;
            break;
        case 4:
            rt = 1.f;
            break;
        case 5:
            rt = 3.f / 2.f;
            break;
        case 6:
            rt = 2.f;
            break;
        case 7:
            rt = 3.f;
            break;
        case 8:
            rt = 4.f;
            break;
        default:
            rt = 1.f;
            break;
        }

        syncRate = rt;
    }
};

#endif // OBXF_SRC_ENGINE_LFO_H
