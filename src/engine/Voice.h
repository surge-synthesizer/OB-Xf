/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
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

    /* Base values (in native units) used by the matrix to compute per-voice adjustments.
     * Set by SynthEngine process* functions alongside the global voice setter. */
    float lfo2BaseRate{0.f};
    float filterEnvAttackBase{1.f}; // ms, matches logsc(0, 1, 60000, 900) default
    float filterEnvReleaseBase{1.f};
    float ampEnvAttackBase{4.f}; // ms, matches logsc(0, 4, 60000, 900) default
    float ampEnvReleaseBase{8.f};

    DelayLine<B_SAMPLES * OVERSAMPLE_FACTOR, float> ampEnvDelayed, filterEnvDelayed, lfo1Delayed,
        lfo2Delayed;

    // output samples per engine block; HQ runs two sub-samples per output sample
    static constexpr int blockSize{32};
    static constexpr int maxBlockSubSamples{blockSize * OVERSAMPLE_FACTOR};

    struct BlockBuffers
    {
        float modEnv[maxBlockSubSamples];
        float ampEnv[maxBlockSubSamples];
        float lfo2[maxBlockSubSamples];
        float notePlaying[maxBlockSubSamples];
        float osc1PitchMod[maxBlockSubSamples];
        float osc2PitchMod[maxBlockSubSamples];
        float osc1PWMod[maxBlockSubSamples];
        float osc2PWMod[maxBlockSubSamples];
        float cutoff[maxBlockSubSamples];
        float out[maxBlockSubSamples];
    } block;

    int blockSubSamples{0};
    bool soundingAtBlockStart{false};

    Voice()
    {
        slop.level = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.ampEnv = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.filterEnv = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.cutoff = juce::Random::getSystemRandom().nextFloat() - 0.5f;
        slop.portamento = juce::Random::getSystemRandom().nextFloat() - 0.5f;
    }

    ~Voice() {}

    // for repeatable renders in tests
    void seedRandom(int64_t seed)
    {
        juce::Random rng(seed);
        slop.level = rng.nextFloat() - 0.5f;
        slop.ampEnv = rng.nextFloat() - 0.5f;
        slop.filterEnv = rng.nextFloat() - 0.5f;
        slop.cutoff = rng.nextFloat() - 0.5f;
        slop.portamento = rng.nextFloat() - 0.5f;
        lfo2.seedRandom(seed + 1);
    }

    void initTuning(Tuning *t) { tuning = t; }

    /*
     * A block runs in four passes, each over every sub-sample this voice plays:
     * processEnvelopes, processControl, processOscillators, processFinish. Sub-samples
     * are output samples, or two per output sample in HQ mode. Within a block nothing
     * changes the voice from outside, so a voice plays a prefix of the block and
     * stops at the first sub-sample its amp envelope is already silent.
     */

    // envelopes run first, since how long each voice plays decides the block's shape
    int processEnvelopes(int subSamples)
    {
        soundingAtBlockStart = sounding;

        const float filterEnvAttack =
            matrixAdjustments.nativeOr(MatrixTarget::FilterEnvAttack, filterEnvAttackBase);
        const float filterEnvRelease =
            matrixAdjustments.nativeOr(MatrixTarget::FilterEnvRelease, filterEnvReleaseBase);
        const float ampEnvAttack =
            matrixAdjustments.nativeOr(MatrixTarget::AmpEnvAttack, ampEnvAttackBase);
        const float ampEnvRelease =
            matrixAdjustments.nativeOr(MatrixTarget::AmpEnvRelease, ampEnvReleaseBase);

        int s = 0;

        for (; s < subSamples; ++s)
        {
            if (!updateSoundingState())
            {
                break;
            }

            filterEnv.applyMatrixAttack(filterEnvAttack);
            filterEnv.applyMatrixRelease(filterEnvRelease);

            block.modEnv[s] = par.filter.invertEnvScale * filterEnv.processSample() *
                              (1 - (1 - velocity) * par.extmod.velToFilter);

            ampEnv.applyMatrixAttack(ampEnvAttack);
            ampEnv.applyMatrixRelease(ampEnvRelease);

            block.ampEnv[s] = ampEnvDelayed.feedReturn(ampEnv.processSample() *
                                                       (1 - (1 - velocity) * par.extmod.velToAmp));
        }

        blockSubSamples = s;

        // retain the velocity-scaled amp envelope value, for voice status LEDs
        if (s > 0)
        {
            ampEnvLevel = block.ampEnv[s - 1];
        }

        return s;
    }

    // per-output-sample cutoff and bend only reach voices sounding as that sample starts
    bool takesEngineValues(int s, int subPerSample) const
    {
        return s % subPerSample == 0 && (s > 0 || soundingAtBlockStart);
    }

    // the sample after a voice stops still hands it the engine's values, and they stick
    void takeTrailingEngineValues(int numSamples, int subPerSample, const float *cutoff,
                                  const float *bend, const float *resonance,
                                  const float *resonanceUnmodulated, const float *multimode)
    {
        if (blockSubSamples % subPerSample != 0)
        {
            return;
        }

        const int t = blockSubSamples / subPerSample;

        if (t >= numSamples || !takesEngineValues(blockSubSamples, subPerSample))
        {
            return;
        }

        par.filter.cutoff = cutoff[t];
        pitchBend = bend[t];
        setFilterEngineValues(resonance[t], resonanceUnmodulated[t], multimode[t]);
    }

    void setFilterEngineValues(float resonance, float resonanceUnmodulated, float multimode)
    {
        /* Resonance is both log-scaled and smoothed, so the smoother carries the normalized
         * value and the curve is applied per voice. Voices with no resonance modulation
         * share the unmodulated result, so only modulated ones pay for the scaling. */
        constexpr auto resScale = matrixTargetScaling(MatrixTarget::FilterResonance);
        const auto resMod = matrixAdjustments.modFor(MatrixTarget::FilterResonance);

        filter.setResonance(resMod == 0.f ? resonanceUnmodulated
                                          : resScale.apply(resonance + resMod));
        filter.setMultimode(multimode);
    }

    void processControl(const float *lfo1In, const float *vibratoLFOIn, const float *cutoff,
                        const float *bend, int subPerSample)
    {
        const float lfo1Amt1 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO1ModAmount1, par.lfo1.amt1);
        const float lfo1Amt2 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO1ModAmount2, par.lfo1.amt2);
        const float lfo2Amt1 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO2ModAmount1, par.lfo2.amt1);
        const float lfo2Amt2 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO2ModAmount2, par.lfo2.amt2);
        const float lfo2Rate =
            juce::jmax(0.01f, matrixAdjustments.nativeOr(MatrixTarget::LFO2Rate, lfo2BaseRate));

        for (int s = 0; s < blockSubSamples; ++s)
        {
            if (takesEngineValues(s, subPerSample))
            {
                par.filter.cutoff = cutoff[s / subPerSample];
                pitchBend = bend[s / subPerSample];
            }

            lfo2.setRate(lfo2Rate);
            lfo2.update();

            float lfo2In = lfo2.getVal();
            block.lfo2[s] = lfo2In;

            double tunedNote = tuning->tunedMidiNote(midiNote);

            // portamento processing (implements RC circuit)
            float portaProcessed = tpt_lp_unwarped(
                state.portamento, tunedNote - 93, // why -93? beats me!
                par.osc.portamento * (1 + slop.portamento * par.slop.portamento), sampleRateInv);

            // rescale pitch bend
            float pitchBendScaled = ((pitchBend < 0.f) ? (pitchBend * par.extmod.pbDown)
                                                       : (pitchBend * par.extmod.pbUp)) +
                                    mpeBend;

            block.notePlaying[s] = portaProcessed;

            // envelope and LFO applied to the filter need a delay equal to internal oscillator
            // delay
            float filterLFO1Mod = lfo1Delayed.feedReturn(lfo1In[s]);
            float filterLFO2Mod = lfo2Delayed.feedReturn(lfo2In);

            float modEnv = block.modEnv[s];

            // with juce::Random this was swinging ~[-1.75, 1.75]
            // but our Noise class swings ~[-0.52, 0.52], so a factor of 3.365 retains old behavior
            float noisyCutoff = noiseGen.getWhite() * 3.365f;

            /* Cutoff's base is smoothed per sample, so it has no entry in VoiceMatrixBases and
             * the modulation is applied here instead. The curve is linear, so adding the mod
             * in native units and clamping the parameter is exactly scale(clamp01(base + mod)).
             * Only the parameter is clamped — LFO, envelope and keytrack still range freely. */
            constexpr auto cutoffScale = matrixTargetScaling(MatrixTarget::FilterCutoff);
            const float modulatedCutoff = juce::jlimit(
                cutoffScale.nativeMin, cutoffScale.nativeMax,
                par.filter.cutoff +
                    matrixAdjustments.modFor(MatrixTarget::FilterCutoff) * cutoffScale.span());

            const float cutoffPitch =
                getPitch((par.lfo1.cutoff * filterLFO1Mod * lfo1Amt1) +
                         (par.lfo2.cutoff * filterLFO2Mod * lfo2Amt1) + modulatedCutoff +
                         slop.cutoff * par.slop.cutoff +
                         par.filter.envAmt * filterEnvDelayed.feedReturn(modEnv) - 45 +
                         (par.filter.keytrack * (pitchBendScaled + portaProcessed + 40)));

            // limit max cutoff for numerical stability
            float cutoffcalc = std::min(cutoffPitch + noisyCutoff, (sampleRate * 0.5f - 120.0f));

            // limit our max cutoff on self-oscillation to prevent aliasing
            if (par.filter.push2Pole)
            {
                cutoffcalc = std::min(cutoffcalc, 19000.f + (5000.f * par.oversample));
            }

            block.cutoff[s] = cutoffcalc;

            // pulse width modulation
            float pwenv = modEnv * (oscs.par.mod.envToPWInvert ? -1 : 1);

            block.osc1PWMod[s] = (par.lfo1.osc1PW * lfo1In[s] * lfo1Amt2) +
                                 (par.lfo2.osc1PW * lfo2In * lfo2Amt2) +
                                 (par.osc.envPWBothOscs ? (par.osc.envPWAmt * pwenv) : 0);
            block.osc2PWMod[s] =
                (par.lfo1.osc2PW * lfo1In[s] * lfo1Amt2) + (par.lfo2.osc2PW * lfo2In * lfo2Amt2) +
                (par.osc.envPWAmt * pwenv) +
                matrixAdjustments.nativeOr(MatrixTarget::Osc2PWOffset, par.osc.pwOsc2Offset);

            // pitch modulation
            float pitchEnv = modEnv * (oscs.par.mod.envToPitchInvert ? -1 : 1);

            /* Pitch is injected into the modulation bus rather than read back from a parameter
             * field, and it is deliberately unclamped, so it goes in as a native delta. The
             * curve is linear, which makes that identical to scale(base + mod) - scale(base). */
            constexpr auto pitchSpan = matrixTargetScaling(MatrixTarget::OscPitch).span();
            const float bothPitchMod = matrixAdjustments.modFor(MatrixTarget::OscPitch) * pitchSpan;

            block.osc1PitchMod[s] =
                (!par.extmod.pbOsc2Only ? pitchBendScaled : 0) +
                (par.lfo1.osc1Pitch * lfo1In[s] * lfo1Amt1) +
                (par.lfo2.osc1Pitch * lfo2In * lfo2Amt1) +
                (par.osc.envPitchBothOscs ? (par.osc.envPitchAmt * pitchEnv) : 0) +
                vibratoLFOIn[s] + matrixAdjustments.modFor(MatrixTarget::Osc1Pitch) * pitchSpan +
                bothPitchMod;
            block.osc2PitchMod[s] = pitchBendScaled + (par.lfo1.osc2Pitch * lfo1In[s] * lfo1Amt1) +
                                    (par.lfo2.osc2Pitch * lfo2In * lfo2Amt1) +
                                    (par.osc.envPitchAmt * pitchEnv) + vibratoLFOIn[s] +
                                    matrixAdjustments.modFor(MatrixTarget::Osc2Pitch) * pitchSpan +
                                    bothPitchMod;
        }
    }

    void processOscillators()
    {
        // apply matrix adjustments to oscillator mix, detune, PW, crossmod, unison for the
        // block; save and restore so they never accumulate
        const float savedOsc1 = oscs.par.mix.osc1;
        const float savedOsc2 = oscs.par.mix.osc2;
        const float savedNoise = oscs.par.mix.noise;
        const float savedRingMod = oscs.par.mix.ringMod;
        const float savedDetune = oscs.par.osc.detune;
        const float savedUnisonDetune = oscs.par.pitch.unisonDetune;
        const float savedOscPW = oscs.par.osc.pw;
        const float savedCrossmod = oscs.par.osc.crossmod;

        oscs.par.mix.osc1 = matrixAdjustments.nativeOr(MatrixTarget::Osc1Vol, oscs.par.mix.osc1);
        oscs.par.mix.osc2 = matrixAdjustments.nativeOr(MatrixTarget::Osc2Vol, oscs.par.mix.osc2);
        oscs.par.mix.noise = matrixAdjustments.nativeOr(MatrixTarget::NoiseVol, oscs.par.mix.noise);
        oscs.par.mix.ringMod =
            matrixAdjustments.nativeOr(MatrixTarget::RingModVol, oscs.par.mix.ringMod);
        oscs.par.osc.detune =
            matrixAdjustments.nativeOr(MatrixTarget::Osc2Detune, oscs.par.osc.detune);
        oscs.par.pitch.unisonDetune =
            matrixAdjustments.nativeOr(MatrixTarget::UnisonDetune, oscs.par.pitch.unisonDetune);
        oscs.par.osc.pw = matrixAdjustments.nativeOr(MatrixTarget::OscPW, oscs.par.osc.pw);
        oscs.par.osc.crossmod =
            matrixAdjustments.nativeOr(MatrixTarget::OscCrossmod, oscs.par.osc.crossmod);

        for (int s = 0; s < blockSubSamples; ++s)
        {
            oscs.par.pitch.notePlaying = block.notePlaying[s];
            oscs.par.mod.osc1PWMod = block.osc1PWMod[s];
            oscs.par.mod.osc2PWMod = block.osc2PWMod[s];
            oscs.par.mod.osc1PitchMod = block.osc1PitchMod[s];
            oscs.par.mod.osc2PitchMod = block.osc2PitchMod[s];

            block.out[s] = oscs.ProcessSample() * (1 - par.slop.level * slop.level);
        }

        oscs.par.mix.osc1 = savedOsc1;
        oscs.par.mix.osc2 = savedOsc2;
        oscs.par.mix.noise = savedNoise;
        oscs.par.mix.ringMod = savedRingMod;
        oscs.par.osc.detune = savedDetune;
        oscs.par.pitch.unisonDetune = savedUnisonDetune;
        oscs.par.osc.pw = savedOscPW;
        oscs.par.osc.crossmod = savedCrossmod;
    }

    void processFinish(const float *lfo1In, const float *resonance,
                       const float *resonanceUnmodulated, const float *multimode, int subPerSample)
    {
        const float lfo1Amt2 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO1ModAmount2, par.lfo1.amt2);
        const float lfo2Amt2 =
            matrixAdjustments.nativeOr(MatrixTarget::LFO2ModAmount2, par.lfo2.amt2);

        for (int s = 0; s < blockSubSamples; ++s)
        {
            if (takesEngineValues(s, subPerSample))
            {
                const auto t = s / subPerSample;
                setFilterEngineValues(resonance[t], resonanceUnmodulated[t], multimode[t]);
            }

            float oscSample = block.out[s];

            // process oscillator brightness
            oscSample = oscSample - tpt_lp_unwarped(state.oscBlock, oscSample, 12, sampleRateInv);
            oscSample = tpt_process(state.brightness, oscSample, state.brightnessCoef);

            // apply filter
            oscSample = par.filter.fourPole ? filter.apply4Pole(oscSample, block.cutoff[s])
                                            : filter.apply2Pole(oscSample, block.cutoff[s]);

            // LFO outputs bipolar values and we need to be unipolar for amplitude modulation,
            // hence the * 0.5 + 0.5
            // LFO's Mod Amount 2 parameter is scaled [0, 0.7], but we need the full [0, 1] swing
            // here, hence the 1.42857... correction factor
            // We also conditionally invert the LFO input because we're subtracting from full
            // volume and we don't want this to *increase* volume
            oscSample *= 1.f - (par.lfo1.volume * lfo1In[s] * 0.5f + par.lfo1.absVolume * 0.5f) *
                                   (lfo1Amt2 * 1.4285714285714286f);

            oscSample *=
                1.f - (par.lfo2.volume * block.lfo2[s] * 0.5f + par.lfo2.absVolume * 0.5f) *
                          (lfo2Amt2 * 1.4285714285714286f);

            oscSample *= block.ampEnv[s];

            block.out[s] = oscSample;
        }
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
        /* Strike is known at note-on time; set it now so Motherboard can
         * recalculateMatrix immediately after this call with a consistent state.
         * Works correctly for the reuseVelocitySentinel path too, since
         * velocity holds its preserved value by this point. */
        setMatrixSource(matrixSourceValues, MatrixSource::Strike, velocity);

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
        setMatrixSource(matrixSourceValues, MatrixSource::Lift, releaseVelocity);

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
