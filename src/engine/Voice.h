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
#include "VoiceMatrix.h"

class Voice
{
  private:
    float sampleRate{1.f};
    float sampleRateInv{1.f};

    // float volume;
    // float port;

    float velocity{0.f};
    bool gated{false};
    bool gatedWithSustain{false};

    float ampEnvLevel{0.f};
    bool sounding{false};

  public:
    int voiceIndex{-1};

  private:
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
            float keytrack{0.f};

            float envAmt{0.f};
            bool invertEnv{false};
            float invertEnvScale{1.f}; // -1 if invertEnv +1 if not

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
            float absVolume{0.f};

        } lfo1, lfo2;

        bool oversample{false};
    } par;

    int midiNote{60};
    int16_t channel{0};
    VoiceMatrixAdjustments matrixAdjustments{};
    VoiceMatrixSourceValues matrixSourceValues{};
    float pitchBend{0.f};
    float mpeBend{0.f};
    bool sustainHold{false};

    float lfo1In{0.f};
    float vibratoLFOIn{0.f};

    /* Base values (in native units) used by the matrix to compute per-voice adjustments.
     * Set by SynthEngine process* functions alongside the global voice setter. */
    float lfo2BaseRate{0.f};
    float filterEnvAttackBase{1.f}; // ms, matches logsc(0, 1, 60000, 900) default
    float filterEnvReleaseBase{1.f};
    float ampEnvAttackBase{4.f}; // ms, matches logsc(0, 4, 60000, 900) default
    float ampEnvReleaseBase{8.f};

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

    inline float ProcessSample(const VoiceMatrix &voiceMatrix)
    {
        // Apply per-voice LFO2 rate offset before updating
        lfo2.setRate(juce::jmax(0.01f, lfo2BaseRate + matrixAdjustments.lfo2Rate *
                                                          VoiceMatrixRanges::lfo2Rate));
        lfo2.update();

        float lfo2In = lfo2.getVal();
        /* Recalculate every sample only when LFO2 is wired into the matrix —
         * otherwise recalculation happens at note/MPE events (see Motherboard).
         * NOTE: when per-sample matrix smoothing is added, recalculation will
         * be needed every sample regardless, and this guard should be modified.
         */
        if (voiceMatrix.isLFO2Bound)
        {
            matrixSourceValues.lfo2 = lfo2In;
            recalculateMatrix(voiceMatrix, matrixSourceValues, matrixAdjustments);
        }

        double tunedNote = tuning->tunedMidiNote(midiNote);

        // portamento processing (implements RC circuit)
        float portaProcessed = tpt_lp_unwarped(
            state.portamento, tunedNote - 93, // why -93? beats me!
            par.osc.portamento * (1 + slop.portamento * par.slop.portamento), sampleRateInv);

        // rescale pitch bend
        float pitchBendScaled =
            ((pitchBend < 0.f) ? (pitchBend * par.extmod.pbDown) : (pitchBend * par.extmod.pbUp)) +
            mpeBend;

        oscs.par.pitch.notePlaying = portaProcessed;

        // envelope and LFO applied to the filter need a delay equal to internal oscillator delay
        float filterLFO1Mod = lfo1Delayed.feedReturn(lfo1In);
        float filterLFO2Mod = lfo2Delayed.feedReturn(lfo2In);

        // apply per-voice LFO mod-amount adjustments (save/restore to avoid accumulation)
        const float savedLfo1Amt1 = par.lfo1.amt1;
        const float savedLfo1Amt2 = par.lfo1.amt2;
        const float savedLfo2Amt1 = par.lfo2.amt1;
        const float savedLfo2Amt2 = par.lfo2.amt2;
        par.lfo1.amt1 = juce::jmax(0.f, par.lfo1.amt1 + matrixAdjustments.lfo1Mod1 *
                                                            VoiceMatrixRanges::lfo1Mod1);
        par.lfo1.amt2 = juce::jlimit(
            0.f, 0.7f, par.lfo1.amt2 + matrixAdjustments.lfo1Mod2 * VoiceMatrixRanges::lfo1Mod2);
        par.lfo2.amt1 = juce::jmax(0.f, par.lfo2.amt1 + matrixAdjustments.lfo2Mod1 *
                                                            VoiceMatrixRanges::lfo2Mod1);
        par.lfo2.amt2 = juce::jlimit(
            0.f, 0.7f, par.lfo2.amt2 + matrixAdjustments.lfo2Mod2 * VoiceMatrixRanges::lfo2Mod2);

        // apply per-voice filter envelope timing adjustments before processSample
        if (matrixAdjustments.filterEnvAttack != 0.f)
            filterEnv.applyMatrixAttack(
                juce::jmax(1.f, filterEnvAttackBase + matrixAdjustments.filterEnvAttack *
                                                          VoiceMatrixRanges::filterEnvAttack));
        if (matrixAdjustments.filterEnvRelease != 0.f)
            filterEnv.applyMatrixRelease(
                juce::jmax(1.f, filterEnvReleaseBase + matrixAdjustments.filterEnvRelease *
                                                           VoiceMatrixRanges::filterEnvRelease));

        // filter envelope
        float modEnv = par.filter.invertEnvScale * filterEnv.processSample() *
                       (1 - (1 - velocity) * par.extmod.velToFilter);

        // filter cutoff calculation

        // with juce::Random this was swinging ~[-1.75, 1.75]
        // but our Noise class swings ~[-0.52, 0.52], so a factor of 3.365 retains old behavior
        float noisyCutoff = noiseGen.getWhite() * 3.365f;

        const float cutoffPitch =
            getPitch((par.lfo1.cutoff * filterLFO1Mod * par.lfo1.amt1) +
                     (par.lfo2.cutoff * filterLFO2Mod * par.lfo2.amt1) + par.filter.cutoff +
                     slop.cutoff * par.slop.cutoff +
                     par.filter.envAmt * filterEnvDelayed.feedReturn(modEnv) - 45 +
                     (par.filter.keytrack * (pitchBendScaled + oscs.par.pitch.notePlaying + 40)) +
                     matrixAdjustments.filterCutoff * VoiceMatrixRanges::filterCutoff);

        // limit max cutoff for numerical stability
        float cutoffcalc = std::min(cutoffPitch + noisyCutoff, (sampleRate * 0.5f - 120.0f));

        // limit our max cutoff on self-oscillation to prevent aliasing
        if (par.filter.push2Pole)
        {
            cutoffcalc = std::min(cutoffcalc, 19000.f + (5000.f * par.oversample));
        }

        // pulse width modulation
        float pwenv = modEnv * (oscs.par.mod.envToPWInvert ? -1 : 1);

        oscs.par.mod.osc1PWMod = (par.lfo1.osc1PW * lfo1In * par.lfo1.amt2) +
                                 (par.lfo2.osc1PW * lfo2In * par.lfo2.amt2) +
                                 (par.osc.envPWBothOscs ? (par.osc.envPWAmt * pwenv) : 0);
        oscs.par.mod.osc2PWMod = (par.lfo1.osc2PW * lfo1In * par.lfo1.amt2) +
                                 (par.lfo2.osc2PW * lfo2In * par.lfo2.amt2) +
                                 (par.osc.envPWAmt * pwenv) + par.osc.pwOsc2Offset +
                                 matrixAdjustments.osc2PWOffset * VoiceMatrixRanges::osc2PWOffset;

        // pitch modulation
        float pitchEnv = modEnv * (oscs.par.mod.envToPitchInvert ? -1 : 1);

        oscs.par.mod.osc1PitchMod =
            (!par.extmod.pbOsc2Only ? pitchBendScaled : 0) +
            (par.lfo1.osc1Pitch * lfo1In * par.lfo1.amt1) +
            (par.lfo2.osc1Pitch * lfo2In * par.lfo2.amt1) +
            (par.osc.envPitchBothOscs ? (par.osc.envPitchAmt * pitchEnv) : 0) + vibratoLFOIn +
            matrixAdjustments.osc1Pitch * VoiceMatrixRanges::osc1Pitch +
            matrixAdjustments.oscPitch * VoiceMatrixRanges::oscPitch;
        oscs.par.mod.osc2PitchMod =
            pitchBendScaled + (par.lfo1.osc2Pitch * lfo1In * par.lfo1.amt1) +
            (par.lfo2.osc2Pitch * lfo2In * par.lfo2.amt1) + (par.osc.envPitchAmt * pitchEnv) +
            vibratoLFOIn + matrixAdjustments.osc2Pitch * VoiceMatrixRanges::osc2Pitch +
            matrixAdjustments.oscPitch * VoiceMatrixRanges::oscPitch;

        // apply matrix adjustments to oscillator mix, detune, PW, crossmod, unison before
        // processing; save and restore to avoid per-sample accumulation
        const float savedOsc1 = oscs.par.mix.osc1;
        const float savedOsc2 = oscs.par.mix.osc2;
        const float savedNoise = oscs.par.mix.noise;
        const float savedRingMod = oscs.par.mix.ringMod;
        const float savedNoiseColor = oscs.par.mix.noiseColor;
        const float savedDetune = oscs.par.osc.detune;
        const float savedUnisonDetune = oscs.par.pitch.unisonDetune;
        const float savedOscPW = oscs.par.osc.pw;
        const float savedCrossmod = oscs.par.osc.crossmod;

        oscs.par.mix.osc1 = juce::jmax(0.f, oscs.par.mix.osc1 + matrixAdjustments.osc1Vol *
                                                                    VoiceMatrixRanges::osc1Vol);
        oscs.par.mix.osc2 = juce::jmax(0.f, oscs.par.mix.osc2 + matrixAdjustments.osc2Vol *
                                                                    VoiceMatrixRanges::osc2Vol);
        oscs.par.mix.noise = juce::jmax(0.f, oscs.par.mix.noise + matrixAdjustments.noiseVol *
                                                                      VoiceMatrixRanges::noiseVol);
        oscs.par.mix.ringMod =
            juce::jmax(0.f, oscs.par.mix.ringMod +
                                matrixAdjustments.ringModVol * VoiceMatrixRanges::ringModVol);
        oscs.par.mix.noiseColor = juce::jlimit(
            0.f, 1.f,
            oscs.par.mix.noiseColor + matrixAdjustments.noiseColor * VoiceMatrixRanges::noiseColor);
        oscs.par.osc.detune =
            oscs.par.osc.detune +
            matrixAdjustments.osc2Detune *
                VoiceMatrixRanges::osc2Detune; // NOTE: detune is log-scaled by SynthEngine;
                                               // adjustment is additive in that space
        oscs.par.pitch.unisonDetune =
            juce::jmax(0.001f, oscs.par.pitch.unisonDetune + matrixAdjustments.unisonDetune *
                                                                 VoiceMatrixRanges::unisonDetune);
        oscs.par.osc.pw = juce::jlimit(
            0.f, 0.95f, oscs.par.osc.pw + matrixAdjustments.oscPW * VoiceMatrixRanges::oscPW);
        oscs.par.osc.crossmod = juce::jmax(
            0.f, oscs.par.osc.crossmod + matrixAdjustments.crossmod * VoiceMatrixRanges::crossmod);

        // process oscillator block
        float oscSample = oscs.ProcessSample() * (1 - par.slop.level * slop.level);

        oscs.par.mix.osc1 = savedOsc1;
        oscs.par.mix.osc2 = savedOsc2;
        oscs.par.mix.noise = savedNoise;
        oscs.par.mix.ringMod = savedRingMod;
        oscs.par.mix.noiseColor = savedNoiseColor;
        oscs.par.osc.detune = savedDetune;
        oscs.par.pitch.unisonDetune = savedUnisonDetune;
        oscs.par.osc.pw = savedOscPW;
        oscs.par.osc.crossmod = savedCrossmod;

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
        oscSample *= 1.f - (par.lfo1.volume * lfo1In * 0.5f + par.lfo1.absVolume * 0.5f) *
                               (par.lfo1.amt2 * 1.4285714285714286f);

        oscSample *= 1.f - (par.lfo2.volume * lfo2In * 0.5f + par.lfo2.absVolume * 0.5f) *
                               (par.lfo2.amt2 * 1.4285714285714286f);

        // restore LFO mod amounts
        par.lfo1.amt1 = savedLfo1Amt1;
        par.lfo1.amt2 = savedLfo1Amt2;
        par.lfo2.amt1 = savedLfo2Amt1;
        par.lfo2.amt2 = savedLfo2Amt2;

        // apply per-voice amp envelope timing adjustments before processSample
        if (matrixAdjustments.ampEnvAttack != 0.f)
            ampEnv.applyMatrixAttack(
                juce::jmax(1.f, ampEnvAttackBase + matrixAdjustments.ampEnvAttack *
                                                       VoiceMatrixRanges::ampEnvAttack));
        if (matrixAdjustments.ampEnvRelease != 0.f)
            ampEnv.applyMatrixRelease(
                juce::jmax(1.f, ampEnvReleaseBase + matrixAdjustments.ampEnvRelease *
                                                        VoiceMatrixRanges::ampEnvRelease));

        // amp envelope
        float ampEnvVal = ampEnvDelayed.feedReturn(ampEnv.processSample() *
                                                   (1 - (1 - velocity) * par.extmod.velToAmp));

        oscSample *= ampEnvVal;

        // retain the velocity-scaled amp envelope value, for voice status LEDs
        ampEnvLevel = ampEnvVal;

        return oscSample;
    }

    void setNoiseColor(float val)
    {
        if (val < 1.f / 3.f)
            oscs.par.mix.noiseColor = OscillatorBlock::White;
        else if (val < 2.f / 3.f)
            oscs.par.mix.noiseColor = OscillatorBlock::Pink;
        else
            oscs.par.mix.noiseColor = OscillatorBlock::Red;
    }

    void setBrightness(float val)
    {
        par.osc.brightness = val;
        state.brightnessCoef =
            tan(std::min(par.osc.brightness, (sampleRate * 0.5f) - 10) * pi * sampleRateInv);
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
    bool isGatedWithSustain() { return gatedWithSustain; }
    bool isGatedOrSustainPedaled() { return gated || gatedWithSustain; }

    void ResetEnvelope()
    {
        ampEnv.ResetEnvelopeState();
        filterEnv.ResetEnvelopeState();
    }

    static constexpr float reuseVelocitySentinel{-0.5f};

    void NoteOn(int note, float vel, int16_t chan)
    {
        OBLOG(voiceManager, "idx=" << voiceIndex << ": Note On " << note << " sound=" << sounding
                                   << " sustainHold=" << sustainHold << " gated=" << gated
                                   << " gatedWithSustain=" << gatedWithSustain)
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
        channel = chan;
        mpeBend = 0.f;
        matrixAdjustments.clear();
        matrixSourceValues.clear();
        /* Velocity is known at note-on time; set it now so Motherboard can
         * recalculateMatrix immediately after this call with a consistent state.
         * Works correctly for the reuseVelocitySentinel path too, since
         * velocity holds its preserved value by this point. */
        setMatrixSource(matrixSourceValues, MatrixSource::Velocity, velocity);

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
        gatedWithSustain = false; // only when released am I sustain gated
    }

    void NoteOff(float releaseVelocity)
    {
        OBLOG(voiceManager, "idx=" << voiceIndex << ": Note Off " << midiNote
                                   << " sound=" << sounding << " sustainHold=" << sustainHold
                                   << " gated=" << gated
                                   << " gatedWithSustain=" << gatedWithSustain)

        /* Set release velocity source; leave other sources (bend, pressure, timbre) intact
         * so they remain active through the release phase. */
        setMatrixSource(matrixSourceValues, MatrixSource::ReleaseVelocity, releaseVelocity);

        if (!sustainHold)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
        }

        gated = false;
        gatedWithSustain = isSounding() && sustainHold;
    }

    void sustOn() { sustainHold = true; }

    void sustOff()
    {
        sustainHold = false;
        gatedWithSustain = false;

        if (!gated)
        {
            ampEnv.triggerRelease();
            filterEnv.triggerRelease();
            gatedWithSustain = false;
        }
    }
};

#endif // OBXF_SRC_ENGINE_VOICE_H
