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
    Motherboard synth;
    Smoother cutoffSmoother;
    Smoother resSmoother;
    Smoother filterModeSmoother;
    Smoother pitchWheelSmoother;
    Smoother modWheelSmoother;
    float sampleRate;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)

  public:
    SynthEngine()
        : cutoffSmoother(), resSmoother(), filterModeSmoother(), pitchWheelSmoother(),
          modWheelSmoother()
    {
    }

    ~SynthEngine() {}

    void setPlayHead(float bpm, float retrPos) { synth.globalLFO.hostSyncRetrigger(bpm, retrPos); }
    void setSampleRate(float sr)
    {
        sampleRate = sr;
        cutoffSmoother.setSampleRate(sr);
        resSmoother.setSampleRate(sr);
        filterModeSmoother.setSampleRate(sr);
        pitchWheelSmoother.setSampleRate(sr);
        modWheelSmoother.setSampleRate(sr);
        synth.setSampleRate(sr);
    }

    void processSample(float *left, float *right)
    {
        processFilterCutoffSmoothed(cutoffSmoother.smoothStep());
        processFilterResonanceSmoothed(resSmoother.smoothStep());
        processFilterModeSmoothed(filterModeSmoother.smoothStep());
        processPitchWheelSmoothed(pitchWheelSmoother.smoothStep());
        processModWheelSmoothed(modWheelSmoother.smoothStep());

        synth.processSample(left, right);
    }

    float getVoiceStatus(uint8_t idx) { return synth.voices[idx].getVoiceStatus(); };

    void allNotesOff()
    {
        for (int i = 0; i < 128; i++)
        {
            processNoteOff(i, -1, 0.f);
        }
    }

#define ForEachVoice(expr)                                                                         \
    for (int i = 0; i < MAX_VOICES; i++)                                                           \
    {                                                                                              \
        synth.voices[i].expr;                                                                      \
    }

    void allSoundOff()
    {
        allNotesOff();

        ForEachVoice(ResetEnvelope());
    }

    void sustainOn() { synth.sustainOn(); }
    void sustainOff() { synth.sustainOff(); }

    void processLFO1Sync(float val)
    {
        if (val > 0.5f)
            synth.globalLFO.setSynced();
        else
            synth.globalLFO.setUnsynced();
    }
    void processNotePriority(float val)
    {
        if (val < 0.33333333f)
            synth.voicePriority = Motherboard::LATEST;
        else if (val < 0.666666666f)
            synth.voicePriority = Motherboard::LOWEST;
        else
            synth.voicePriority = Motherboard::HIGHEST;
    }
    void processNoteOn(int noteNo, float velocity, int8_t channel)
    {
        synth.setNoteOn(noteNo, velocity, channel);
    }
    void processNoteOff(int noteNo, float velocity, int8_t channel)
    {
        synth.setNoteOff(noteNo, velocity, channel);
    }
    void processEcoMode(float val) { synth.economyMode = val > 0.5f; }
    void processVelToAmpEnv(float val) { ForEachVoice(vamp = val); }
    void processVelToFilterEnv(float val) { ForEachVoice(vflt = val); }
    void processModWheel(float val) { modWheelSmoother.setStep(val); }
    void processModWheelSmoothed(float val) { synth.vibratoAmount = val; }
    void processVibratoLFORate(float val) { synth.vibratoLFO.setFrequency(linsc(val, 2.f, 12.f)); }
    void processVibratoLFOWave(float val)
    {
        synth.vibratoLFO.wave1blend = val > 0.5f ? 0.f : -1.f;
        synth.vibratoLFO.wave2blend = val > 0.5f ? -1.f : 0.f;
    }
    void processPitchWheel(float val) { pitchWheelSmoother.setStep(val); }
    inline void processPitchWheelSmoothed(float val) { ForEachVoice(pitchWheel = val); }
    void processPolyphony(float val)
    {
        synth.setPolyphony(juce::roundToInt((val * (MAX_VOICES - 1)) + 1.f));
    }
    void processUnisonVoices(float val)
    {
        synth.setUnisonVoices(juce::roundToInt((val * (MAX_PANNINGS - 1)) + 1.f));
    }
    void processBendUpRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(pitchWheelUpAmt = v);
    }
    void processBendDownRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(pitchWheelDownAmt = v);
    }
    void processBendOsc2Only(float val) { ForEachVoice(pitchWheelOsc2Only = val); }
    void processPan(float val, int idx) { synth.pannings[(idx - 1) % MAX_PANNINGS] = val; }
    void processTune(float val)
    {
        const auto v = val * 2.f - 1.f;
        ForEachVoice(osc.tune = v);
    }
    void processEnvLegatoMode(float val)
    {
        const auto v = juce::roundToInt(val * 3.f + 1.f) - 1;
        ForEachVoice(legatoMode = v);
    }
    void processTranspose(float val)
    {
        const auto v = juce::roundToInt(((val * 2.f) - 1.f) * 24.f);
        ForEachVoice(osc.oct = v);
    }
    void processFilterKeyFollow(float val) { ForEachVoice(fltKF = val); }
    void processFilter2PolePush(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(selfOscPush = v);
        ForEachVoice(flt.selfOscPush = v);
    }
    void processFilter4PoleXpander(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(flt.xpander = v);
    }
    void processFilterXpanderMode(float val)
    {
        const auto v = juce::jmin(val * 15.f, 14.f);
        ForEachVoice(flt.xpanderMode = v);
    }
    void processUnison(float val) { synth.uni = val > 0.5f; }
    void processPortamento(float val)
    {
        const auto v = logsc(1.f - val, 0.14f, 250.f, 150.f);
        ForEachVoice(porta = v);
    }
    void processVolume(float val) { synth.Volume = linsc(val, 0.f, 0.30f); }
    void processLFO1Rate(float val)
    {
        synth.globalLFO.setRawParam(val);
        synth.globalLFO.setFrequency(logsc(val, 0.f, 250.f, 3775.f));
    }
    void processLFO1Wave1(float val) { synth.globalLFO.wave1blend = linsc(val, -1.f, 1.f); }
    void processLFO1Wave2(float val) { synth.globalLFO.wave2blend = linsc(val, -1.f, 1.f); }
    void processLFO1Wave3(float val) { synth.globalLFO.wave3blend = linsc(val, -1.f, 1.f); }
    void processLFO1PW(float val) { synth.globalLFO.pw = val; }
    void processLFO1ModAmount1(float val)
    {
        const auto v = logsc(logsc(val, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f);
        ForEachVoice(lfoa1 = v);
    }
    void processLFO1ModAmount2(float val)
    {
        const auto v = linsc(val, 0.f, 0.7f);
        ForEachVoice(lfoa2 = v);
    }
    void processLFO1ToOsc1Pitch(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfoo1 = v);
    }
    void processLFO1ToOsc2Pitch(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfoo2 = v);
    }
    void processLFO1ToFilterCutoff(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfof = v);
    }
    void processLFO1ToOsc1PW(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfopw1 = v);
    }
    void processLFO1ToOsc2PW(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfopw2 = v);
    }
    void processLFO1ToVolume(float val)
    {
        const auto v = (val == 0.5f ? 1.f : ((val == 1.f) ? -1.f : 0.f));
        ForEachVoice(lfovol = v);
    }
    void processUnisonDetune(float val)
    {
        const auto v = logsc(val, 0.001f, 1.f);
        ForEachVoice(osc.totalDetune = v);
    }
    void processOscPW(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(osc.pulseWidth = v);
    }
    // PW env range is actually 0.95f, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 0.95f by a reciprocal of 0.9 here
    void processEnvToPWAmount(float val)
    {
        const auto v = linsc(val, 0.f, 1.055555555555555f);
        ForEachVoice(pwenvmod = v);
    }
    void processOsc2PWOffset(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(pwOfs = v);
    }
    void processEnvToPWBothOscs(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(pwEnvBoth = v);
    }
    void processFilterEnvInvert(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(invertFenv = v);
    }
    void processPitchBothOscs(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(pitchModBoth = v);
    }
    void processCrossmod(float val)
    {
        const auto v = val * 48.f;
        ForEachVoice(osc.xmod = v);
    }
    // pitch env range is actually 36 st, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 36 semitones by a reciprocal of 0.9 here
    void processEnvToPitchAmount(float val)
    {
        const auto v = (val * 40.f);
        ForEachVoice(envpitchmod = v);
    }
    void processOscSync(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.hardSync = v);
    }
    void processOsc1Pitch(float val)
    {
        const auto v = (val * 48.f);
        ForEachVoice(osc.osc1p = v);
    }
    void processOsc2Pitch(float val)
    {
        const auto v = (val * 48.f);
        ForEachVoice(osc.osc2p = v);
    }
    void processEnvToPitchInvert(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.penvinv = v);
    }
    void processEnvToPWInvert(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.pwenvinv = v);
    }
    void processOsc1Mix(float val) { ForEachVoice(osc.osc1Mix = val); }
    void processOsc2Mix(float val) { ForEachVoice(osc.osc2Mix = val); }
    void processRingModMix(float val) { ForEachVoice(osc.ringModMix = val); }
    void processNoiseMix(float val) { ForEachVoice(osc.noiseMix = val); }
    void processNoiseColor(float val) { ForEachVoice(osc.noiseColor = val); }
    void processOscBrightness(float val)
    {
        const auto v = linsc(val, 7000.f, 26000.f);
        ForEachVoice(setBrightness(v));
    }
    void processOsc2Detune(float val)
    {
        const auto v = logsc(val, 0.001f, 0.6f);
        ForEachVoice(osc.osc2Det = v);
    }
    void processOsc1Saw(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.osc1Saw = v);
    }
    void processOsc1Pulse(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.osc1Pul = v);
    }
    void processOsc2Saw(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.osc2Saw = v);
    }
    void processOsc2Pulse(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.osc2Pul = v);
    }
    void processFilterCutoff(float val) { cutoffSmoother.setStep(linsc(val, 0.f, 120.f)); }
    inline void processFilterCutoffSmoothed(float val) { ForEachVoice(cutoff = val); }
    void processFilterResonance(float val)
    {
        resSmoother.setStep(0.991f - logsc(1.f - val, 0.f, 0.991f, 40.f));
    }
    inline void processFilterResonanceSmoothed(float val) { ForEachVoice(flt.setResonance(val)); }
    void processFilter2PoleBPBlend(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(flt.bandPassSw = v);
    }
    void processFilter4PoleMode(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(fourpole = v);
    }
    void processFilterMode(float val) { filterModeSmoother.setStep(val); }
    inline void processFilterModeSmoothed(float val) { ForEachVoice(flt.setMultimode(val)); }
    void processHQMode(float val) { synth.SetOversample(val > 0.5f); }
    void processFilterEnvAmount(float val)
    {
        const auto v = linsc(val, 0.f, 140.f);
        ForEachVoice(fenvamt = v);
    }
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
        ForEachVoice(FltSlopAmt = v);
    }
    void processPortamentoSlop(float val)
    {
        const auto v = linsc(val, 0.f, 0.75f);
        ForEachVoice(PortaSlopAmt = v);
    }
    void processLevelSlop(float val)
    {
        const auto v = linsc(val, 0.f, 0.67f);
        ForEachVoice(levelSlopAmt = v);
    }
};

#endif // OBXF_SRC_ENGINE_SYNTHENGINE_H
