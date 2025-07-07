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
#include "Filter.h"
#include "Decimator.h"
#include "APInterpolator.h"
#include "Tuning.h"

class Voice
{
  private:
    float sampleRate;
    float sampleRateInv;
    // float volume;
    // float port;
    float status;
    float velocity;

    float oscState, brightProcState;

    Tuning *tuning;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Voice)

  public:
    bool sustainHold;
    // bool resetOnAttack;

    ADSREnvelope ampEnv;
    ADSREnvelope filterEnv;
    OscillatorBlock osc;
    Filter filter;

    juce::Random noiseGen;

    float velToAmp, velToFilter;

    float cutoff;
    float filterEnvAmt;

    float envSlop;
    float filterEnvSlop;

    float filterSlop;
    float filterSlopAmt;

    float portaSlop;
    float portaSlopAmt;

    float levelSlop;
    float levelSlopAmt;

    float bright;
    float brightCoef;

    int midiNote;

    bool active;
    bool shouldProcess;

    float filterKeyfollow;

    float portamento;
    float portamentoState;

    float lfoIn;
    float lfoVibratoIn;

    float pitchBend;
    float pitchBendUpAmt;
    float pitchBendDownAmt;
    bool pitchBendOsc2Only;

    float lfo1amt1, lfo1amt2;

    int8_t lfo1osc1pitch, lfo1osc2pitch, lfo1cutoff;
    int8_t lfo1osc1pw, lfo1osc2pw, lfo1volume;

    bool oversample;

    float envPitchMod;
    float envPWMod;

    int envLegatoMode;
    bool invertFilterEnv;

    float pwOsc2Offset;
    bool pwEnvBoth;
    bool pitchEnvBoth;

    bool selfOscPush;
    bool filter4pole;

    DelayLine<Samples * 2> ampEnvDelayed, filterEnvDelayed, lfo1Delayed;
    ApInterpolator ap;

    Voice() : ap()
    {
        selfOscPush = false;
        pitchEnvBoth = false;
        pwOsc2Offset = 0.f;
        invertFilterEnv = false;
        pwEnvBoth = false;
        noiseGen = juce::Random(juce::Random::getSystemRandom().nextInt64());
        sustainHold = false;
        shouldProcess = false;
        velToAmp = velToFilter = 0.f;
        velocity = 0.f;
        lfoVibratoIn = 0.f;
        filter4pole = false;
        envLegatoMode = 0;
        brightCoef = bright = 1.f;
        envPitchMod = 0.f;
        envPWMod = 0.f;
        oversample = false;
        oscState = brightProcState = 0.f;
        pitchBend = pitchBendUpAmt = pitchBendDownAmt = 0.f;
        lfoIn = 0.f;
        portaSlopAmt = 0.f;
        filterSlopAmt = 0.f;
        levelSlopAmt = 0.f;
        portamento = 0.f;
        portamentoState = 0.f;
        filterKeyfollow = 0.f;
        cutoff = 0.f;
        filterEnvAmt = 0.f;
        active = false;
        midiNote = 60;
        levelSlop = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        envSlop = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        filterEnvSlop = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        filterSlop = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        portaSlop = juce::Random::getSystemRandom().nextFloat() - 0.5f;
    }

    ~Voice() {}

    void initTuning(Tuning *t) { tuning = t; }

    inline float ProcessSample()
    {
        double tunedNote = tuning->tunedMidiNote(midiNote);

        // portamento processing (implements RC circuit)
        float portaProcessed =
            tpt_lp_unwarped(portamentoState, tunedNote - 93,
                            portamento * (1 + portaSlop * portaSlopAmt), sampleRateInv);

        // rescale pitch bend
        float pitchBendScaled =
            (pitchBend < 0.f) ? (pitchBend * pitchBendDownAmt) : (pitchBend * pitchBendUpAmt);

        osc.notePlaying = portaProcessed;

        // envelope and LFO applied to the filter need a delay equal to internal oscillator delay
        float filterLFOMod = lfo1Delayed.feedReturn(lfoIn);

        // filter envelope
        float modEnv = filterEnv.processSample() * (1 - (1 - velocity) * velToFilter);

        if (invertFilterEnv)
        {
            modEnv = -modEnv;
        }

        // filter cutoff calculation
        float cutoffcalc = juce::jmin(
            getPitch((lfo1cutoff * filterLFOMod * lfo1amt1) + cutoff + filterSlop * filterSlopAmt +
                     filterEnvAmt * filterEnvDelayed.feedReturn(modEnv) - 45 +
                     (filterKeyfollow * (pitchBendScaled + osc.notePlaying + 40)))
                // noisy filter cutoff
                + (noiseGen.nextFloat() - 0.5f) * 3.5f,
            (filter.sampleRate * 0.5f - 120.0f)); // limit max cutoff for numerical stability

        // limit our max cutoff on self oscillation to prevent aliasing
        if (selfOscPush)
        {
            cutoffcalc = juce::jmin(cutoffcalc, 19000.f + (5000.f * oversample));
        }

        // pulse width modulation
        float pwenv = modEnv * (osc.pwenvinv ? -1 : 1);

        osc.pw1 = (lfo1osc1pw * lfoIn * lfo1amt2) + (pwEnvBoth ? (envPWMod * pwenv) : 0);
        osc.pw2 = (lfo1osc2pw * lfoIn * lfo1amt2) + (envPWMod * pwenv) + pwOsc2Offset;

        // pitch modulation
        float pitchEnv = modEnv * (osc.penvinv ? -1 : 1);

        osc.pto1 = (!pitchBendOsc2Only ? pitchBendScaled : 0) + (lfo1osc1pitch * lfoIn * lfo1amt1) +
                   (pitchEnvBoth ? (envPitchMod * pitchEnv) : 0) + lfoVibratoIn;
        osc.pto2 = pitchBendScaled + (lfo1osc2pitch * lfoIn * lfo1amt1) + (envPitchMod * pitchEnv) +
                   lfoVibratoIn;

        // process oscillator block
        float oscSample = osc.ProcessSample() * (1 - levelSlopAmt * levelSlop);

        // process oscillator brightness
        oscSample = oscSample - tpt_lp_unwarped(oscState, oscSample, 12, sampleRateInv);
        oscSample = tpt_process(brightProcState, oscSample, brightCoef);

        // apply filter
        oscSample = filter4pole ? filter.apply4Pole(oscSample, cutoffcalc)
                                : filter.apply2Pole(oscSample, cutoffcalc);

        // LFO outputs bipolar values and we need to be unipolar for amplitude modulation
        // LFO's Mod Amount 2 parameter is scaled [0, 0.7], but we need the full [0, 1] swing here
        // We also invert the LFO input because we're subtracting from full volume
        // If we don't do that, sawtooth would act as a ramp, and we don't want that
        oscSample *= 1.f - (lfo1volume * (-lfoIn * 0.5f + 0.5f) * (lfo1amt2 * 1.4285714285714286f));

        // amp envelope
        float ampEnvVal =
            ampEnvDelayed.feedReturn(ampEnv.processSample() * (1 - (1 - velocity) * velToAmp));

        oscSample *= ampEnvVal;

        // retain the velocity-scaled amp envelope value, for voice status LEDs
        status = ampEnvVal;

        return oscSample;
    }

    void setBrightness(float val)
    {
        bright = val;
        brightCoef =
            tan(juce::jmin(val, filter.sampleRate * 0.5f - 10) * pi * filter.sampleRateInv);
    }

    void setEnvTimingOffset(float d)
    {
        ampEnv.setUniqueOffset(1 + envSlop * d);
        filterEnv.setUniqueOffset(1 + filterEnvSlop * d);
    }

    void setHQ(bool hq)
    {
        if (hq)
        {
            osc.setDecimation();
        }
        else
        {
            osc.removeDecimation();
        }

        oversample = hq;
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1 / sr;

        filter.setSampleRate(sr);
        osc.setSampleRate(sr);
        ampEnv.setSampleRate(sr);
        filterEnv.setSampleRate(sr);

        brightCoef = tan(juce::jmin(bright, filter.sampleRate * 0.5f - 10) *
                         (juce::MathConstants<float>::pi) * filter.sampleRateInv);
    }

    void checkEnvelopeState() { shouldProcess = ampEnv.isActive(); }

    float getVoiceStatus() { return shouldProcess * status; }

    void ResetEnvelope()
    {
        ampEnv.ResetEnvelopeState();
        filterEnv.ResetEnvelopeState();
    }

    static constexpr float reuseVelocitySentinel{-0.5f};

    void NoteOn(int note, float vel)
    {
        if (!shouldProcess)
        {
            // When your processing is paused we need to clear delay lines and envelopes
            // Not doing this will cause clicks or glitches
            ampEnvDelayed.fillZeroes();
            filterEnvDelayed.fillZeroes();

            ResetEnvelope();
        }

        shouldProcess = true;

        if (vel > reuseVelocitySentinel)
        {
            velocity = vel;
        }

        midiNote = note;

        if (!active || (envLegatoMode & 1))
        {
            ampEnv.triggerAttack();
        }

        if (!active || (envLegatoMode & 2))
        {
            filterEnv.triggerAttack();
        }

        active = true;
    }

    void NoteOff()
    {
        if (!sustainHold)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
        }

        active = false;
    }

    void sustOn() { sustainHold = true; }

    void sustOff()
    {
        sustainHold = false;

        if (!active)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
        }
    }
};

#endif // OBXF_SRC_ENGINE_VOICE_H
