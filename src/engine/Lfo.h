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
    float sampleRate{1.f};
    float sampleRateInv{1.f};

    struct State
    {
        float phase{0.f};
        float phaseInc{0.f};

        bool tempoSynced{false};
        float syncedRate{1.f};
        float unsyncedRate{0.f};
        float rawSyncedRate{0.f};

        float smoothedOutput{0.f};

        juce::Random rng;

        struct Waves
        {
            float sine{0.f};
            float square{0.f};
            float saw{0.f};
            float tri{0.f};
            float samplehold{0.f};
            float sampleglide{0.f};

            float history{0.f};
        } wave;
    } state;

  public:
    struct Parameters
    {
        bool unipolarPulse{false};

        float pw{0.f};

        float wave1blend{0.f};
        float wave2blend{0.f};
        float wave3blend{0.f};
    } par;

    LFO()
    {
        state.rng = juce::Random();
        state.wave.samplehold = state.rng.nextFloat() * 2.f - 1.f;
        state.wave.history = state.wave.samplehold;
    }

    void setTempoSync(const bool ts)
    {
        if (ts)
        {
            state.tempoSynced = true;
            recalcRate(state.rawSyncedRate);
        }
        else
        {
            state.tempoSynced = false;
            state.phaseInc = state.unsyncedRate;
        }
    }

    void hostSyncRetrigger(float bpm, float quarters, bool resetPosition)
    {
        if (state.tempoSynced)
        {
            state.phaseInc = (bpm / 60.f) * state.syncedRate;
            if (resetPosition)
            {
                state.phase = state.phaseInc * quarters;
                state.phase -= fmod(state.phase, 1.f) * twoPi - pi;
            }
        }
    }

    inline float getVal()
    {
        float result = 0.f;

        if (par.wave1blend >= 0.f)
        {
            result += state.wave.tri * par.wave1blend;
        }
        else
        {
            result += state.wave.sine * -par.wave1blend;
        }

        if (par.wave2blend >= 0.f)
        {
            result += state.wave.saw * par.wave2blend;
        }
        else
        {
            result += state.wave.square * -par.wave2blend;
        }

        if (par.wave3blend >= 0.f)
        {
            result += state.wave.sampleglide * par.wave3blend;
        }
        else
        {
            result += state.wave.samplehold * -par.wave3blend;
        }

        return tpt_lp_unwarped(state.smoothedOutput, result, 250.f, sampleRateInv);
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
        state.phase += ((state.phaseInc * twoPi * sampleRateInv));

        if (state.phase > pi)
        {
            state.phase -= twoPi;
            state.wave.history = state.wave.samplehold;
            state.wave.samplehold = state.rng.nextFloat() * 2.f - 1.f;
        }

        // casting dance is to satisfy MSVC Clang
        state.wave.sine =
            static_cast<float>(juce::dsp::FastMathApproximations::sin<double>(state.phase));
        state.wave.tri =
            (twoByPi * abs(state.phase + halfPi - (state.phase > halfPi) * twoPi)) - 1.f;
        state.wave.square = (state.phase > (pi * par.pw * 0.9f) ? -1.f + par.unipolarPulse : 1.f);
        state.wave.saw = bend(-state.phase * invPi, -par.pw);
        state.wave.sampleglide = state.wave.history + (state.wave.samplehold - state.wave.history) *
                                                          (pi + state.phase) * invTwoPi;
    }

    // valid input value range is 0...1, otherwise phase will not be set!
    void setPhaseDirectly(float val)
    {
        if (val >= 0.f && val <= 1.f)
        {
            state.phase = (val * twoPi) - pi;
        }
    }

    void setRate(float val)
    {
        state.unsyncedRate = val;

        if (!state.tempoSynced)
        {
            state.phaseInc = val;
        }
    }

    // used for tempo-synced rate changes
    void setRateNormalized(float param)
    {
        state.rawSyncedRate = param;

        if (state.tempoSynced)
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

        state.syncedRate = rt;
    }
};

#endif // OBXF_SRC_ENGINE_LFO_H
