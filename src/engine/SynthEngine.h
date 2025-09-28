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

#ifndef OBXF_SRC_ENGINE_SYNTHENGINE_H
#define OBXF_SRC_ENGINE_SYNTHENGINE_H

#include <core/Constants.h>
#include "Voice.h"
#include "Motherboard.h"
#include "Parameters.h"
#include "Smoother.h"

class SynthEngine
{
  private:
#define ForEachVoice(expr)                                                                         \
    for (int i = 0; i < MAX_VOICES; i++)                                                           \
    {                                                                                              \
        synth.voices[i].expr;                                                                      \
    }

    Motherboard synth;
    Smoother cutoffSmoother;
    Smoother resSmoother;
    Smoother filterModeSmoother;
    Smoother pitchBendSmoother;
    Smoother modWheelSmoother;

    float sampleRate;

    // clever trick to avoid nested ternary, which provides 0.f -> 0.f, 0.5f -> 1.f, 1.f -> -1.f
    // we use it for inverting LFO modulations per target via tri-state buttons
    float remapZeroHalfOneToZeroOneMinusOne(float x)
    {
        auto res = (5 * x) - (6 * x * x);
        // if x is, like, 0.05 from a vst edge  will blow out so round to restore ternary
        return std::round(res);
    }

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)

  public:
    SynthEngine()
        : cutoffSmoother(), resSmoother(), filterModeSmoother(), pitchBendSmoother(),
          modWheelSmoother()
    {
    }

    ~SynthEngine() {}

    void setPlayHead(float bpm, float retrPos, bool resetPosition)
    {
        synth.globalLFO.hostSyncRetrigger(bpm, retrPos, resetPosition);
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;

        cutoffSmoother.setSampleRate(sr);
        resSmoother.setSampleRate(sr);
        filterModeSmoother.setSampleRate(sr);
        pitchBendSmoother.setSampleRate(sr);
        modWheelSmoother.setSampleRate(sr);
        synth.setSampleRate(sr);
    }

    void processSample(float *left, float *right)
    {
        processFilterCutoffSmoothed(cutoffSmoother.smoothStep());
        processFilterResonanceSmoothed(resSmoother.smoothStep());
        processFilterModeSmoothed(filterModeSmoother.smoothStep());
        processPitchWheelSmoothed(pitchBendSmoother.smoothStep());
        processModWheelSmoothed(modWheelSmoother.smoothStep());

        synth.processSample(left, right);
    }

    float getVoiceAmpEnvStatus(uint8_t idx) { return synth.voices[idx].getVoiceAmpEnvStatus(); };

    Motherboard *getMotherboard() { return &synth; };

    void processNoteOn(int note, float velocity, int8_t channel)
    {
        synth.setNoteOn(note, velocity, channel);
    }

    void processNoteOff(int note, float velocity, int8_t channel)
    {
        synth.setNoteOff(note, velocity, channel);
    }

    void allSoundOff()
    {
        allNotesOff();

        ForEachVoice(ResetEnvelope());
    }

    void sustainOn() { synth.sustainOn(); }

    void sustainOff() { synth.sustainOff(); }

    void allNotesOff()
    {
        for (int i = 0; i < 128; i++)
        {
            processNoteOff(i, -1, 0.f);
        }
    }

    void processPitchWheel(float val) { pitchBendSmoother.setStep(val); }

    inline void processPitchWheelSmoothed(float val) { ForEachVoice(pitchBend = val); }

    void processModWheel(float val) { modWheelSmoother.setStep(val); }

    void processModWheelSmoothed(float val) { synth.vibratoAmount = val; }

    void processNotePriority(float val)
    {
        const int priority = static_cast<int>(val * 2.f);

        switch (priority)
        {
        case 0:
        default:
            synth.voicePriority = Motherboard::LATEST;
            break;
        case 1:
            synth.voicePriority = Motherboard::LOWEST;
            break;
        case 2:
            synth.voicePriority = Motherboard::HIGHEST;
            break;
        }
    }
    void processVelToAmpEnv(float val) { ForEachVoice(par.extmod.velToAmp = val); }
    void processVelToFilterEnv(float val) { ForEachVoice(par.extmod.velToFilter = val); }
    void processVibratoLFORate(float val) { synth.vibratoLFO.setRate(linsc(val, 2.f, 12.f)); }
    void processVibratoLFOWave(float val)
    {
        synth.vibratoLFO.par.wave1blend = val >= 0.5f ? 0.f : -1.f;
        synth.vibratoLFO.par.wave2blend = val >= 0.5f ? -1.f : 0.f;
    }
    void processPolyphony(float val)
    {
        const int voices = 1 + static_cast<int>(val * MAX_VOICES);
        synth.setPolyphony(voices);
    }

    void processUnisonVoices(float val)
    {
        const int voices = 1 + static_cast<int>(val * MAX_PANNINGS);
        synth.setUnisonVoices(voices);
    }

    void processBendUpRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(par.extmod.pbUp = v);
    }
    void processBendDownRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(par.extmod.pbDown = v);
    }
    void processBendOsc2Only(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(par.extmod.pbOsc2Only = v);
    }
    void processPan(float val, int idx) { synth.pannings[(idx - 1) % MAX_PANNINGS] = val; }
    void processTune(float val)
    {
        const auto v = val * 2.f - 1.f;
        ForEachVoice(oscs.par.pitch.tune = v);
    }
    void processEnvLegatoMode(float val)
    {
        const int mode = static_cast<int>(val * 3.f);
        ForEachVoice(par.extmod.envLegatoMode = mode);
    }
    void processTranspose(float val)
    {
        const auto v = juce::roundToInt(((val * 2.f) - 1.f) * 24.f);
        ForEachVoice(oscs.par.pitch.transpose = v);
    }
    void processFilterKeyFollow(float val) { ForEachVoice(par.filter.keyfollow = val); }
    void processFilter2PolePush(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(setFilter2PolePush(v));
    }
    void processFilter4PoleXpander(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(filter.par.xpander4Pole = v);
    }
    void processFilterXpanderMode(float val)
    {
        const auto v = juce::roundToInt(val * (NUM_XPANDER_MODES - 1));
        ForEachVoice(filter.par.xpanderMode = v);
    }
    void processUnison(float val) { synth.unison = val >= 0.5f; }
    void processPortamento(float val)
    {
        const auto v = logsc(1.f - val, 0.14f, 250.f, 150.f);
        ForEachVoice(par.osc.portamento = v);
    }
    void processVolume(float val) { synth.volume = linsc(val, 0.f, 0.30f); }
    void processLFO1Rate(float val)
    {
        synth.globalLFO.setRate(logsc(val, 0.f, 250.f, 3775.f));
        synth.globalLFO.setRateNormalized(val);
    }
    void processLFO1Sync(float val) { synth.globalLFO.setTempoSync(val >= 0.5f); }
    void processLFO1Wave1(float val) { synth.globalLFO.par.wave1blend = linsc(val, -1.f, 1.f); }
    void processLFO1Wave2(float val) { synth.globalLFO.par.wave2blend = linsc(val, -1.f, 1.f); }
    void processLFO1Wave3(float val) { synth.globalLFO.par.wave3blend = linsc(val, -1.f, 1.f); }
    void processLFO1PW(float val) { synth.globalLFO.par.pw = val; }
    void processLFO1ModAmount1(float val)
    {
        const auto v = logsc(logsc(val, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f);
        ForEachVoice(par.lfo1.amt1 = v);
    }
    void processLFO1ModAmount2(float val)
    {
        const auto v = linsc(val, 0.f, 0.7f);
        ForEachVoice(par.lfo1.amt2 = v);
    }
    void processLFO1ToOsc1Pitch(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.osc1Pitch = v);
    }
    void processLFO1ToOsc2Pitch(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.osc2Pitch = v);
    }
    void processLFO1ToFilterCutoff(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.cutoff = v);
    }
    void processLFO1ToOsc1PW(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.osc1PW = v);
    }
    void processLFO1ToOsc2PW(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.osc2PW = v);
    }
    void processLFO1ToVolume(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo1.volume = v);
    }

    void processLFO2Rate(float val)
    {
        const auto v = logsc(val, 0.f, 250.f, 3775.f);
        ForEachVoice(lfo2.setRate(v));
        ForEachVoice(lfo2.setRateNormalized(val));
    }
    void processLFO2Sync(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(lfo2.setTempoSync(v));
    }
    void processLFO2Wave1(float val) { ForEachVoice(lfo2.par.wave1blend = linsc(val, -1.f, 1.f)); }
    void processLFO2Wave2(float val) { ForEachVoice(lfo2.par.wave2blend = linsc(val, -1.f, 1.f)); }
    void processLFO2Wave3(float val) { ForEachVoice(lfo2.par.wave3blend = linsc(val, -1.f, 1.f)); }
    void processLFO2PW(float val) { ForEachVoice(lfo2.par.pw = val); }
    void processLFO2ModAmount1(float val)
    {
        const auto v = logsc(logsc(val, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f);
        ForEachVoice(par.lfo2.amt1 = v);
    }
    void processLFO2ModAmount2(float val)
    {
        const auto v = linsc(val, 0.f, 0.7f);
        ForEachVoice(par.lfo2.amt2 = v);
    }
    void processLFO2ToOsc1Pitch(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.osc1Pitch = v);
    }
    void processLFO2ToOsc2Pitch(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.osc2Pitch = v);
    }
    void processLFO2ToFilterCutoff(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.cutoff = v);
    }
    void processLFO2ToOsc1PW(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.osc1PW = v);
    }
    void processLFO2ToOsc2PW(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.osc2PW = v);
    }
    void processLFO2ToVolume(float val)
    {
        const auto v = remapZeroHalfOneToZeroOneMinusOne(val);
        ForEachVoice(par.lfo2.volume = v);
    }

    void processUnisonDetune(float val)
    {
        const auto v = logsc(val, 0.001f, 1.f);
        ForEachVoice(oscs.par.pitch.unisonDetune = v);
    }
    void processOscPW(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(oscs.par.osc.pw = v);
    }
    // PW env range is actually 0.95f, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 0.95f by a reciprocal of 0.9 here
    void processEnvToPWAmount(float val)
    {
        const auto v = linsc(val, 0.f, 1.055555555555555f);
        ForEachVoice(par.osc.envPWAmt = v);
    }
    void processOsc2PWOffset(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(par.osc.pwOsc2Offset = v);
    }
    void processEnvToPWBothOscs(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(par.osc.envPWBothOscs = v);
    }
    void processFilterEnvInvert(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(par.filter.invertEnv = v);
    }
    void processPitchBothOscs(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(par.osc.envPitchBothOscs = v);
    }
    void processCrossmod(float val)
    {
        const auto v = val * 48.f;
        ForEachVoice(oscs.par.osc.crossmod = v);
    }
    // pitch env range is actually 36 st, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 36 semitones by a reciprocal of 0.9 here
    void processEnvToPitchAmount(float val)
    {
        const auto v = (val * 40.f);
        ForEachVoice(par.osc.envPitchAmt = v);
    }
    void processOscSync(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.osc.sync = v);
    }
    void processOsc1Pitch(float val)
    {
        const auto v = (val * 48.f);
        ForEachVoice(oscs.par.osc.pitch1 = v);
    }
    void processOsc2Pitch(float val)
    {
        const auto v = (val * 48.f);
        ForEachVoice(oscs.par.osc.pitch2 = v);
    }
    void processEnvToPitchInvert(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.mod.envToPitchInvert = v);
    }
    void processEnvToPWInvert(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.mod.envToPWInvert = v);
    }
    void processOsc1Mix(float val) { ForEachVoice(oscs.par.mix.osc1 = val); }
    void processOsc2Mix(float val) { ForEachVoice(oscs.par.mix.osc2 = val); }
    void processRingModMix(float val) { ForEachVoice(oscs.par.mix.ringMod = val); }
    void processNoiseMix(float val) { ForEachVoice(oscs.par.mix.noise = val); }
    void processNoiseColor(float val) { ForEachVoice(oscs.par.mix.noiseColor = val); }
    void processOscBrightness(float val)
    {
        const auto v = linsc(val, 7000.f, 26000.f);
        ForEachVoice(setBrightness(v));
    }
    void processOsc2Detune(float val)
    {
        const auto v = logsc(val, 0.001f, 0.6f);
        ForEachVoice(oscs.par.osc.detune = v);
    }
    void processOsc1Saw(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.osc.saw1 = v);
    }
    void processOsc1Pulse(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.osc.pulse1 = v);
    }
    void processOsc2Saw(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.osc.saw2 = v);
    }
    void processOsc2Pulse(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(oscs.par.osc.pulse2 = v);
    }
    void processFilterCutoff(float val) { cutoffSmoother.setStep(linsc(val, 0.f, 120.f)); }
    inline void processFilterCutoffSmoothed(float val) { ForEachVoice(par.filter.cutoff = val); }
    void processFilterResonance(float val)
    {
        resSmoother.setStep(0.991f - logsc(1.f - val, 0.f, 0.991f, 40.f));
    }
    inline void processFilterResonanceSmoothed(float val)
    {
        ForEachVoice(filter.setResonance(val));
    }
    void processFilter2PoleBPBlend(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(filter.par.bpBlend2Pole = v);
    }
    void processFilter4PoleMode(float val)
    {
        const auto v = val >= 0.5f;
        ForEachVoice(par.filter.fourPole = v);
    }
    void processFilterMode(float val) { filterModeSmoother.setStep(val); }
    inline void processFilterModeSmoothed(float val) { ForEachVoice(filter.setMultimode(val)); }
    void processHQMode(float val)
    {
        bool v = val > 0.5f;

        if (v != synth.oversample)
        {
            allSoundOff();
        }

        synth.SetHQMode(v);
    }
    void processFilterEnvAmount(float val)
    {
        const auto v = linsc(val, 0.f, 140.f);
        ForEachVoice(par.filter.envAmt = v);
    }
    void processAmpEnvAttackCurve(float val) { ForEachVoice(ampEnv.setAttackCurve(val)); }
    void processAmpEnvAttack(float val)
    {
        const auto v = logsc(val, 4.f, 60000.f, 900.f);
        ForEachVoice(ampEnv.setAttack(v));
    }
    void processAmpEnvDecay(float val)
    {
        const auto v = logsc(val, 4.f, 60000.f, 900.f);
        ForEachVoice(ampEnv.setDecay(v));
    }
    void processAmpEnvSustain(float val) { ForEachVoice(ampEnv.setSustain(val)); }
    void processAmpEnvRelease(float val)
    {
        const auto v = logsc(val, 8.f, 60000.f, 900.f);
        ForEachVoice(ampEnv.setRelease(v));
    }
    void processFilterEnvAttackCurve(float val) { ForEachVoice(filterEnv.setAttackCurve(val)); }
    void processFilterEnvAttack(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(filterEnv.setAttack(v));
    }
    void processFilterEnvDecay(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(filterEnv.setDecay(v));
    }
    void processFilterEnvSustain(float val) { ForEachVoice(filterEnv.setSustain(val)); }
    void processFilterEnvRelease(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(filterEnv.setRelease(v));
    }
    void processEnvelopeSlop(float val) { ForEachVoice(setEnvTimingOffset(val)); }
    void processFilterSlop(float val)
    {
        const auto v = linsc(val, 0.f, 18.f);
        ForEachVoice(par.slop.cutoff = v);
    }
    void processPortamentoSlop(float val)
    {
        const auto v = linsc(val, 0.f, 0.75f);
        ForEachVoice(par.slop.portamento = v);
    }
    void processLevelSlop(float val)
    {
        const auto v = linsc(val, 0.f, 0.67f);
        ForEachVoice(par.slop.level = v);
    }
};

#endif // OBXF_SRC_ENGINE_SYNTHENGINE_H
