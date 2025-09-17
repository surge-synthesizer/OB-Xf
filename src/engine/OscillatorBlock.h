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

#ifndef OBXF_SRC_ENGINE_OSCILLATORBLOCK_H
#define OBXF_SRC_ENGINE_OSCILLATORBLOCK_H

#include "Voice.h"
#include "SynthEngine.h"
#include "AudioUtils.h"
#include "BlepData.h"
#include "DelayLine.h"
#include "Noise.h"
#include "SawOsc.h"
#include "PulseOsc.h"
#include "TriangleOsc.h"

class OscillatorBlock
{
  private:
    static constexpr float oneThird = 1.f / 3.f;
    static constexpr float twoThirds = 2.f / 3.f;

    float sampleRate{1.f};
    float sampleRateInv{1.f};

    struct OscillatorState
    {
        float phase{0.f};
        float pitch{0.f};
        float tuningSlop{0.f};
        float pw{0.f};
    } osc1, osc2;

    struct DelayLines
    {
        DelayLine<B_SAMPLES, bool> sync;
        DelayLine<B_SAMPLES, float> syncFrac;
        DelayLine<B_SAMPLES, float> crossmod;
        DelayLine<B_SAMPLES, float> pitch;
    } delay;

    struct Generators
    {
        Noise noise;
        SawOsc osc1Saw, osc2Saw;
        PulseOsc osc1Pulse, osc2Pulse;
        TriangleOsc osc1Triangle, osc2Triangle;
    } gen;

  public:
    struct Parameters
    {
        struct Pitch
        {
            int transpose{0};
            float tune{0.f};

            float unisonDetune{0.f};

            float notePlaying{60.f};
        } pitch;

        struct Osc
        {
            float pitch1{0.f};
            float pitch2{0.f};

            float detune{0.f};

            float pw{0.f};

            bool saw1{false};
            bool saw2{false};
            bool pulse1{false};
            bool pulse2{false};

            float crossmod{0.f};
            bool sync{false};

        } osc;

        struct Mod
        {
            float oscPitchNoise{0.1f};

            float osc1PitchMod{0.f};
            float osc2PitchMod{0.f};
            float osc1PWMod{0.f};
            float osc2PWMod{0.f};

            bool envToPitchInvert{false};
            bool envToPWInvert{false};
        } mod;

        struct Mixer
        {
            float osc1{0.f};
            float osc2{0.f};
            float ringMod{0.f};
            float noise{0.f};
            float noiseColor{0.f};
        } mix;
    } par;

    OscillatorBlock() = default;
    ~OscillatorBlock() = default;

    void setDecimation()
    {
        gen.osc1Pulse.setDecimation();
        gen.osc1Triangle.setDecimation();
        gen.osc1Saw.setDecimation();
        gen.osc2Pulse.setDecimation();
        gen.osc2Triangle.setDecimation();
        gen.osc2Saw.setDecimation();
    }

    void removeDecimation()
    {
        gen.osc1Pulse.removeDecimation();
        gen.osc1Triangle.removeDecimation();
        gen.osc1Saw.removeDecimation();
        gen.osc2Pulse.removeDecimation();
        gen.osc2Triangle.removeDecimation();
        gen.osc2Saw.removeDecimation();
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.f / sampleRate;

        gen.noise.setSampleRate(sampleRate);
        gen.noise.seedWhiteNoise(std::rand());

        osc1.tuningSlop = gen.noise.getWhite();
        osc2.tuningSlop = gen.noise.getWhite();

        osc1.phase = gen.noise.getWhite();
        osc2.phase = gen.noise.getWhite();
    }

    inline float ProcessSample()
    {
        osc1.pitch = getPitch(par.mod.oscPitchNoise * gen.noise.getWhite() + par.pitch.notePlaying +
                              par.osc.pitch1 + par.mod.osc1PitchMod + par.pitch.tune +
                              par.pitch.transpose + par.pitch.unisonDetune * osc1.tuningSlop);
        bool syncReset = false;
        float syncFrac = 0.f;
        float fs = juce::jmin(osc1.pitch * sampleRateInv, 0.45f);

        osc1.phase += fs;
        syncFrac = 0.f;

        float osc1out = 0.f;
        float pwcalc =
            juce::jlimit<float>(0.1f, 1.f, (par.osc.pw + par.mod.osc1PWMod) * 0.5f + 0.5f);

        if (par.osc.pulse1)
        {
            gen.osc1Pulse.processLeader(osc1.phase, fs, pwcalc, osc1.pw);
        }

        if (par.osc.saw1)
        {
            gen.osc1Saw.processLeader(osc1.phase, fs);
        }
        else if (!par.osc.pulse1)
        {
            gen.osc1Triangle.processLeader(osc1.phase, fs);
        }

        if (osc1.phase >= 1.f)
        {
            osc1.phase -= 1.f;
            syncFrac = osc1.phase / fs;
            syncReset = true;
        }

        osc1.pw = pwcalc;
        syncReset &= par.osc.sync;

        // Delaying our hardsync gate signal and frac
        syncReset = delay.sync.feedReturn(syncReset) != 0.f;
        syncFrac = delay.syncFrac.feedReturn(syncFrac);

        if (par.osc.pulse1)
        {
            osc1out += gen.osc1Pulse.getValue(osc1.phase, pwcalc) + gen.osc1Pulse.aliasReduction();
        }

        if (par.osc.saw1)
        {
            osc1out += gen.osc1Saw.getValue(osc1.phase) + gen.osc1Saw.aliasReduction();
        }
        else if (!par.osc.pulse1)
        {
            osc1out = gen.osc1Triangle.getValue(osc1.phase) + gen.osc1Triangle.aliasReduction();
        }

        // pitch control needs additional delay buffer to compensate
        // this will give us less aliasing on crossmod
        osc2.pitch = getPitch(delay.pitch.feedReturn(
            par.mod.oscPitchNoise * gen.noise.getWhite() + par.pitch.notePlaying + par.osc.detune +
            par.osc.pitch2 + par.mod.osc2PitchMod + osc1out * par.osc.crossmod + par.pitch.tune +
            par.pitch.transpose + par.pitch.unisonDetune * osc2.tuningSlop));

        fs = juce::jmin(osc2.pitch * sampleRateInv, 0.45f);

        pwcalc = juce::jlimit<float>(0.1f, 1.f, (par.osc.pw + par.mod.osc2PWMod) * 0.5f + 0.5f);

        float osc2out = 0.f;

        osc2.phase += fs;

        if (par.osc.pulse2)
        {
            gen.osc2Pulse.processFollower(osc2.phase, fs, syncReset, syncFrac, pwcalc, osc2.pw);
        }

        if (par.osc.saw2)
        {
            gen.osc2Saw.processFollower(osc2.phase, fs, syncReset, syncFrac);
        }
        else if (!par.osc.pulse2)
        {
            gen.osc2Triangle.processFollower(osc2.phase, fs, syncReset, syncFrac);
        }

        if (osc2.phase >= 1.f)
        {
            osc2.phase -= 1.f;
        }

        osc2.pw = pwcalc;

        // On hard sync reset, follower phase is affected that way
        if (syncReset)
        {
            osc2.phase = fs * syncFrac;
        }

        // delaying osc 1 signal and getting delayed back
        osc1out = delay.crossmod.feedReturn(osc1out);

        if (par.osc.pulse2)
        {
            osc2out += gen.osc2Pulse.getValue(osc2.phase, pwcalc) + gen.osc2Pulse.aliasReduction();
        }

        if (par.osc.saw2)
        {
            osc2out += gen.osc2Saw.getValue(osc2.phase) + gen.osc2Saw.aliasReduction();
        }
        else if (!par.osc.pulse2)
        {
            osc2out = gen.osc2Triangle.getValue(osc2.phase) + gen.osc2Triangle.aliasReduction();
        }

        float rmOut = osc1out * osc2out;
        float noise = 0.f;

        if (par.mix.noiseColor < oneThird)
        {
            noise = gen.noise.getWhite();
        }
        else if (par.mix.noiseColor < twoThirds)
        {
            noise = gen.noise.getPink();
        }
        else
        {
            noise = gen.noise.getRed();
        }

        // mixing
        float out = (osc1out * par.mix.osc1) + (osc2out * par.mix.osc2) +
                    (noise * (par.mix.noise + 0.0006f)) + (rmOut * par.mix.ringMod);

        return out * 3.f;
    }
};

#endif // OBXF_SRC_ENGINE_OSCILLATORBLOCK_H