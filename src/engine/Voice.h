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

#ifndef OBXF_SRC_ENGINE_VOICE_H
#define OBXF_SRC_ENGINE_VOICE_H

#include "OscillatorBlock.h"
#include "AdsrEnvelope.h"
#include "Lfo.h"
#include "Filter.h"
#include "Decimator.h"
#include "Tuning.h"

class Voice
{
  private:
    float sampleRate{1.f};
    float sampleRateInv{1.f};

    // float volume;
    // float port;

    float velocity{0.f};
    bool gated{false};

    float ampEnvLevel{0.f};
    bool sounding{false};

    Tuning *tuning;

    struct InternalState
    {
        float oscBlock{0.f};
        float brightness{0.f};
        float brightnessCoef{0.f};
        float portamento{0.f};
    } state;

    struct SlopState
    {
        float ampEnv{0.f};
        float filterEnv{0};
        float cutoff{0.f};
        float portamento{0.f};
        float level{0.f};
    } slop;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)

  public:
    OscillatorBlock oscs;
    Filter filter;
    ADSREnvelope filterEnv;
    ADSREnvelope ampEnv;
    Noise noiseGen;
    LFO lfo2;

    struct Parameters
    {
        struct Slop
        {
            float cutoff{0.f};
            float portamento{0.f};
            float level{0.f};
        } slop;

        struct Osc
        {
            float portamento{0.f};
            float brightness{1.f};

            float pwOsc2Offset{0.f};

            float envPitchAmt{0.f};
            float envPWAmt{0.f};

            bool envPitchBothOscs{true};
            bool envPWBothOscs{true};
        } osc;

        struct Filter
        {
            float cutoff{0.f};
            float keyfollow{0.f};

            float envAmt{0.f};
            bool invertEnv{false};

            bool push2Pole{false};
            bool fourPole{false};
        } filter;

        struct ExtMod
        {
            float pbUp{0.f};
            float pbDown{0.f};
            bool pbOsc2Only{false};

            float velToAmp{0.f};
            float velToFilter{0.f};

            int envLegatoMode{0};
        } extmod;

        struct LFO
        {
            float amt1{0.f};
            float amt2{0.f};

            float osc1Pitch{0.f};
            float osc2Pitch{0.f};
            float cutoff{0.f};
            float osc1PW{0.f};
            float osc2PW{0.f};
            float volume{0.f};

        } lfo1, lfo2;

        bool oversample{false};
    } par;

    int midiNote{60};
    float pitchBend{0.f};
    bool sustainHold{false};

    float lfo1In{0.f};
    float vibratoLFOIn{0.f};

    DelayLine<B_SAMPLES * OVERSAMPLE_FACTOR, float> ampEnvDelayed, filterEnvDelayed, lfo1Delayed,
        lfo2Delayed;

    Voice()
    {
        slop.level = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.ampEnv = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.filterEnv = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.cutoff = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.portamento = juce::Random::getSystemRandom().nextFloat() - 0.5f;
    }

    ~Voice() {}

    void initTuning(Tuning *t) { tuning = t; }

    inline float ProcessSample()
    {
        lfo2.update();

        float lfo2In = lfo2.getVal();

        double tunedNote = tuning->tunedMidiNote(midiNote);

        // portamento processing (implements RC circuit)
        float portaProcessed = tpt_lp_unwarped(
            state.portamento, tunedNote - 93,
            par.osc.portamento * (1 + slop.portamento * par.slop.portamento), sampleRateInv);

        // rescale pitch bend
        float pitchBendScaled =
            (pitchBend < 0.f) ? (pitchBend * par.extmod.pbDown) : (pitchBend * par.extmod.pbUp);

        oscs.par.pitch.notePlaying = portaProcessed;

        // envelope and LFO applied to the filter need a delay equal to internal oscillator delay
        float filterLFO1Mod = lfo1Delayed.feedReturn(lfo1In);
        float filterLFO2Mod = lfo2Delayed.feedReturn(lfo2In);

        // filter envelope
        float modEnv = filterEnv.processSample() * (1 - (1 - velocity) * par.extmod.velToFilter);

        if (par.filter.invertEnv)
        {
            modEnv = -modEnv;
        }

        // filter cutoff calculation

        // with juce::Random this was swinging ~[-1.75, 1.75]
        // but our Noise class swings ~[-0.52, 0.52], so a factor of 3.365 retains old behavior
        float noisyCutoff = noiseGen.getWhite() * 3.365f;

        const float cutoffPitch =
            getPitch((par.lfo1.cutoff * filterLFO1Mod * par.lfo1.amt1) +
                     (par.lfo2.cutoff * filterLFO2Mod * par.lfo2.amt1) + par.filter.cutoff +
                     slop.cutoff * par.slop.cutoff +
                     par.filter.envAmt * filterEnvDelayed.feedReturn(modEnv) - 45 +
                     (par.filter.keyfollow * (pitchBendScaled + oscs.par.pitch.notePlaying + 40)));

        // limit max cutoff for numerical stability
        float cutoffcalc = juce::jmin(cutoffPitch + noisyCutoff, (sampleRate * 0.5f - 120.0f));

        // limit our max cutoff on self-oscillation to prevent aliasing
        if (par.filter.push2Pole)
        {
            cutoffcalc = juce::jmin(cutoffcalc, 19000.f + (5000.f * par.oversample));
        }

        // pulse width modulation
        float pwenv = modEnv * (oscs.par.mod.envToPWInvert ? -1 : 1);

        oscs.par.mod.osc1PWMod = (par.lfo1.osc1PW * lfo1In * par.lfo1.amt2) +
                                 (par.lfo2.osc1PW * lfo2In * par.lfo2.amt2) +
                                 (par.osc.envPWBothOscs ? (par.osc.envPWAmt * pwenv) : 0);
        oscs.par.mod.osc2PWMod = (par.lfo1.osc2PW * lfo1In * par.lfo1.amt2) +
                                 (par.lfo2.osc2PW * lfo2In * par.lfo2.amt2) +
                                 (par.osc.envPWAmt * pwenv) + par.osc.pwOsc2Offset;

        // pitch modulation
        float pitchEnv = modEnv * (oscs.par.mod.envToPitchInvert ? -1 : 1);

        oscs.par.mod.osc1PitchMod =
            (!par.extmod.pbOsc2Only ? pitchBendScaled : 0) +
            (par.lfo1.osc1Pitch * lfo1In * par.lfo1.amt1) +
            (par.lfo2.osc1Pitch * lfo2In * par.lfo2.amt1) +
            (par.osc.envPitchBothOscs ? (par.osc.envPitchAmt * pitchEnv) : 0) + vibratoLFOIn;
        oscs.par.mod.osc2PitchMod = pitchBendScaled +
                                    (par.lfo1.osc2Pitch * lfo1In * par.lfo1.amt1) +
                                    (par.lfo2.osc2Pitch * lfo2In * par.lfo2.amt1) +
                                    (par.osc.envPitchAmt * pitchEnv) + vibratoLFOIn;

        // process oscillator block
        float oscSample = oscs.ProcessSample() * (1 - par.slop.level * slop.level);

        // process oscillator brightness
        oscSample = oscSample - tpt_lp_unwarped(state.oscBlock, oscSample, 12, sampleRateInv);
        oscSample = tpt_process(state.brightness, oscSample, state.brightnessCoef);

        // apply filter
        oscSample = par.filter.fourPole ? filter.apply4Pole(oscSample, cutoffcalc)
                                        : filter.apply2Pole(oscSample, cutoffcalc);

        // LFO outputs bipolar values and we need to be unipolar for amplitude modulation,
        // hence the * 0.5 + 0.5
        // LFO's Mod Amount 2 parameter is scaled [0, 0.7], but we need the full [0, 1] swing here,
        // hence the 1.42857... correction factor
        // We also conditionally invert the LFO input because we're subtracting from full volume
        // and we don't want this to *increase* volume
        if (par.lfo1.volume >= 0)
        {
            oscSample *= 1.f - (par.lfo1.volume * (lfo1In * 0.5f + 0.5f) *
                                (par.lfo1.amt2 * 1.4285714285714286f));
        }
        else
        {
            oscSample *= 1.f - (-lfo1In * 0.5f + 0.5f) * (par.lfo1.amt2 * 1.4285714285714286f);
        }

        if (par.lfo2.volume >= 0)
        {
            oscSample *= 1.f - (par.lfo2.volume * (lfo2In * 0.5f + 0.5f) *
                                (par.lfo2.amt2 * 1.4285714285714286f));
        }
        else
        {
            oscSample *= 1.f - (-lfo2In * 0.5f + 0.5f) * (par.lfo2.amt2 * 1.4285714285714286f);
        }

        // amp envelope
        float ampEnvVal = ampEnvDelayed.feedReturn(ampEnv.processSample() *
                                                   (1 - (1 - velocity) * par.extmod.velToAmp));

        oscSample *= ampEnvVal;

        // retain the velocity-scaled amp envelope value, for voice status LEDs
        ampEnvLevel = ampEnvVal;

        return oscSample;
    }

    void setBrightness(float val)
    {
        par.osc.brightness = val;
        state.brightnessCoef =
            tan(juce::jmin(par.osc.brightness, (sampleRate * 0.5f) - 10) * pi * sampleRateInv);
    }

    void setEnvTimingOffset(float d)
    {
        ampEnv.setEnvOffsets(1.f + slop.ampEnv * d);
        filterEnv.setEnvOffsets(1.f + slop.filterEnv * d);
    }

    void setFilter2PolePush(float d)
    {
        par.filter.push2Pole = d;
        filter.par.push2Pole = d;
    }

    void setHQMode(bool hq)
    {
        if (hq)
        {
            oscs.setDecimation();
        }
        else
        {
            oscs.removeDecimation();
        }

        filter.reset();

        par.oversample = hq;
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1 / sr;

        oscs.setSampleRate(sr);
        filter.setSampleRate(sr);
        filterEnv.setSampleRate(sr);
        lfo2.setSampleRate(sr);
        ampEnv.setSampleRate(sr);
        noiseGen.setSampleRate(sr);
        noiseGen.seedWhiteNoise(std::rand());

        setBrightness(par.osc.brightness);
    }

    bool updateSoundingState()
    {
        sounding = ampEnv.isActive();
        return sounding;
    }

    float getVoiceAmpEnvStatus() { return sounding * ampEnvLevel; }
    bool isSounding() { return sounding; }
    bool isGated() { return gated; }

    void ResetEnvelope()
    {
        ampEnv.ResetEnvelopeState();
        filterEnv.ResetEnvelopeState();
    }

    static constexpr float reuseVelocitySentinel{-0.5f};

    void NoteOn(int note, float vel)
    {
        if (!sounding)
        {
            // When your processing is paused we need to clear delay lines and envelopes
            // Not doing this will cause clicks or glitches
            ampEnvDelayed.fillZeroes();
            filterEnvDelayed.fillZeroes();

            ResetEnvelope();
        }

        sounding = true;

        if (vel > reuseVelocitySentinel)
        {
            velocity = vel;
        }

        midiNote = note;

        if (!gated || (par.extmod.envLegatoMode & 1))
        {
            ampEnv.triggerAttack();
        }

        if (!gated || (par.extmod.envLegatoMode & 2))
        {
            filterEnv.triggerAttack();
        }

        // LFO 2 always starts from phase 0
        lfo2.setPhaseDirectly(0.f);

        gated = true;
    }

    void NoteOff()
    {
        if (!sustainHold)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
        }

        gated = false;
    }

    void sustOn() { sustainHold = true; }

    void sustOff()
    {
        sustainHold = false;

        if (!gated)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
        }
    }
};

#endif // OBXF_SRC_ENGINE_VOICE_H
