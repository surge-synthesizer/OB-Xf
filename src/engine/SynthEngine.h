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
    Smoother multimodeSmoother;
    Smoother pitchWheelSmoother;
    Smoother modWheelSmoother;
    float sampleRate;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)

  public:
    SynthEngine()
        : cutoffSmoother(), resSmoother(), multimodeSmoother(), pitchWheelSmoother(),
          modWheelSmoother()
    {
    }

    ~SynthEngine() {}

    void setPlayHead(float bpm, float retrPos) { synth.mlfo.hostSyncRetrigger(bpm, retrPos); }
    void setSampleRate(float sr)
    {
        sampleRate = sr;
        cutoffSmoother.setSampleRate(sr);
        resSmoother.setSampleRate(sr);
        multimodeSmoother.setSampleRate(sr);
        pitchWheelSmoother.setSampleRate(sr);
        modWheelSmoother.setSampleRate(sr);
        synth.setSampleRate(sr);
    }

    void processSample(float *left, float *right)
    {
        processCutoffSmoothed(cutoffSmoother.smoothStep());
        processResonanceSmoothed(resSmoother.smoothStep());
        processMultimodeSmoothed(multimodeSmoother.smoothStep());
        procPitchWheelSmoothed(pitchWheelSmoother.smoothStep());
        procModWheelSmoothed(modWheelSmoother.smoothStep());

        synth.processSample(left, right);
    }

    void allNotesOff()
    {
        for (int i = 0; i < 128; i++)
        {
            procNoteOff(i, -1, 0.f);
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
    void procLfoSync(float val)
    {
        if (val > 0.5f)
            synth.mlfo.setSynced();
        else
            synth.mlfo.setUnsynced();
    }
    void procNotePriority(float val)
    {
        if (val < 0.33333333f)
            synth.voicePriority = Motherboard::LATEST;
        else if (val < 0.666666666f)
            synth.voicePriority = Motherboard::LOWEST;
        else
            synth.voicePriority = Motherboard::HIGHEST;
    }
    void procNoteOn(int noteNo, float velocity, int8_t channel)
    {
        synth.setNoteOn(noteNo, velocity, channel);
    }
    void procNoteOff(int noteNo, float velocity, int8_t channel)
    {
        synth.setNoteOff(noteNo, velocity, channel);
    }
    void procEconomyMode(float val) { synth.economyMode = val > 0.5f; }
    void procAmpVelocityAmount(float val) { ForEachVoice(vamp = val); }
    void procFltVelocityAmount(float val) { ForEachVoice(vflt = val); }
    void procModWheel(float val) { modWheelSmoother.setStep(val); }
    void procModWheelSmoothed(float val) { synth.vibratoAmount = val; }
    void procModWheelFrequency(float val) { synth.vibratoLfo.setFrequency(linsc(val, 2.f, 12.f)); }
    void procModWheelWave(float val)
    {
        synth.vibratoLfo.wave1blend = val > 0.5f ? 0.f : -1.f;
        synth.vibratoLfo.wave2blend = val > 0.5f ? -1.f : 0.f;
    }
    void procPitchWheel(float val) { pitchWheelSmoother.setStep(val); }
    inline void procPitchWheelSmoothed(float val) { ForEachVoice(pitchWheel = val); }
    void setPolyphony(float val)
    {
        synth.setPolyphony(juce::roundToInt((val * (MAX_VOICES - 1)) + 1.f));
    }
    void setUnisonVoices(float val)
    {
        synth.setUnisonVoices(juce::roundToInt((val * (MAX_PANNINGS - 1)) + 1.f));
    }
    void procPitchBendUpRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(pitchWheelUpAmt = v);
    }
    void procPitchBendDownRange(float val)
    {
        const auto v = val * MAX_BEND_RANGE;
        ForEachVoice(pitchWheelDownAmt = v);
    }
    void procPitchWheelOsc2Only(float val) { ForEachVoice(pitchWheelOsc2Only = val); }
    void processPan(float val, int idx) { synth.pannings[(idx - 1) % MAX_PANNINGS] = val; }
    void processTune(float val)
    {
        const auto v = val * 2.f - 1.f;
        ForEachVoice(osc.tune = v);
    }
    void processLegatoMode(float val)
    {
        const auto v = juce::roundToInt(val * 3.f + 1.f) - 1;
        ForEachVoice(legatoMode = v);
    }
    void processOctave(float val)
    {
        const auto v = juce::roundToInt(((val * 2.f) - 1.f) * 24.f);
        ForEachVoice(osc.oct = v);
    }
    void processFilterKeyFollow(float val) { ForEachVoice(fltKF = val); }
    void processSelfOscPush(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(selfOscPush = v);
        ForEachVoice(flt.selfOscPush = v);
    }
    void processXpanderFilter(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(flt.xpander = v);
    }
    void processXpanderMode(float val)
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
    void processLfoFrequency(float val)
    {
        synth.mlfo.setRawParam(val);
        synth.mlfo.setFrequency(logsc(val, 0.f, 250.f, 3775.f));
    }
    void processLfoSine(float val) { synth.mlfo.wave1blend = linsc(val, -1.f, 1.f); }
    void processLfoSquare(float val) { synth.mlfo.wave2blend = linsc(val, -1.f, 1.f); }
    void processLfoSH(float val) { synth.mlfo.wave3blend = linsc(val, -1.f, 1.f); }
    void processLfoPulsewidth(float val) { synth.mlfo.pw = val; }
    void processLfoAmt1(float val)
    {
        const auto v = logsc(logsc(val, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f);
        ForEachVoice(lfoa1 = v);
    }
    void processLfoOsc1(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfoo1 = v);
    }
    void processLfoOsc2(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfoo2 = v);
    }
    void processLfoFilter(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfof = v);
    }
    void processLfoPw1(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfopw1 = v);
    }
    void processLfoPw2(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfopw2 = v);
    }
    void processLfoVolume(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(lfovol = v);
    }
    void processLfoAmt2(float val)
    {
        const auto v = linsc(val, 0.f, 0.7f);
        ForEachVoice(lfoa2 = v);
    }
    void processDetune(float val)
    {
        const auto v = logsc(val, 0.001f, 1.f);
        ForEachVoice(osc.totalDetune = v);
    }
    void processPulseWidth(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(osc.pulseWidth = v);
    }
    // PW env range is actually 0.95f, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 0.95f by a reciprocal of 0.9 here
    void processPwEnv(float val)
    {
        const auto v = linsc(val, 0.f, 1.055555555555555f);
        ForEachVoice(pwenvmod = v);
    }
    void processPwOfs(float val)
    {
        const auto v = linsc(val, 0.f, 0.95f);
        ForEachVoice(pwOfs = v);
    }
    void processPwEnvBoth(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(pwEnvBoth = v);
    }
    void processInvertFenv(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(invertFenv = v);
    }
    void processPitchModBoth(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(pitchModBoth = v);
    }
    void processOsc2Xmod(float val)
    {
        const auto v = val * 48.f;
        ForEachVoice(osc.xmod = v);
    }
    // pitch env range is actually 36 st, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 36 semitones by a reciprocal of 0.9 here
    void processEnvelopeToPitch(float val)
    {
        const auto v = (val * 40.f);
        ForEachVoice(envpitchmod = v);
    }
    void processOsc2HardSync(float val)
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
    void processEnvelopeToPitchInvert(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.penvinv = v);
    }
    void processEnvelopeToPWInvert(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.pwenvinv = v);
    }
    void processOsc1Mix(float val) { ForEachVoice(osc.osc1Mix = val); }
    void processOsc2Mix(float val) { ForEachVoice(osc.osc2Mix = val); }
    void processRingModMix(float val) { ForEachVoice(osc.ringModMix = val); }
    void processNoiseMix(float val) { ForEachVoice(osc.noiseMix = val); }
    void processNoiseColor(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(osc.noiseColor = v);
    }
    void processBrightness(float val)
    {
        const auto v = linsc(val, 7000.f, 26000.f);
        ForEachVoice(setBrightness(v));
    }
    void processOsc2Det(float val)
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
    void processCutoff(float val) { cutoffSmoother.setStep(linsc(val, 0.f, 120.f)); }
    inline void processCutoffSmoothed(float val) { ForEachVoice(cutoff = val); }
    void processResonance(float val)
    {
        resSmoother.setStep(0.991f - logsc(1.f - val, 0.f, 0.991f, 40.f));
    }
    inline void processResonanceSmoothed(float val) { ForEachVoice(flt.setResonance(val)); }
    void processBandpassSw(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(flt.bandPassSw = v);
    }
    void processFourPole(float val)
    {
        const auto v = val > 0.5f;
        ForEachVoice(fourpole = v);
    }
    void processMultimode(float val) { multimodeSmoother.setStep(val); }
    inline void processMultimodeSmoothed(float val) { ForEachVoice(flt.setMultimode(val)); }
    void processOversampling(float val) { synth.SetOversample(val > 0.5f); }
    void processFilterEnvelopeAmt(float val)
    {
        const auto v = linsc(val, 0.f, 140.f);
        ForEachVoice(fenvamt = v);
    }
    void processLoudnessEnvelopeAttack(float val)
    {
        const auto v = logsc(val, 4.f, 60000.f, 900.f);
        ForEachVoice(env.setAttack(v));
    }
    void processLoudnessEnvelopeDecay(float val)
    {
        const auto v = logsc(val, 4.f, 60000.f, 900.f);
        ForEachVoice(env.setDecay(v));
    }
    void processLoudnessEnvelopeSustain(float val) { ForEachVoice(env.setSustain(val)); }
    void processLoudnessEnvelopeRelease(float val)
    {
        const auto v = logsc(val, 8.f, 60000.f, 900.f);
        ForEachVoice(env.setRelease(v));
    }
    void processFilterEnvelopeAttack(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(fenv.setAttack(v));
    }
    void processFilterEnvelopeDecay(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(fenv.setDecay(v));
    }
    void processFilterEnvelopeSustain(float val) { ForEachVoice(fenv.setSustain(val)); }
    void processFilterEnvelopeRelease(float val)
    {
        const auto v = logsc(val, 1.f, 60000.f, 900.f);
        ForEachVoice(fenv.setRelease(v));
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
    void processLoudnessSlop(float val)
    {
        const auto v = linsc(val, 0.f, 0.67f);
        ForEachVoice(levelSlopAmt = v);
    }
};

#endif // OBXF_SRC_ENGINE_SYNTHENGINE_H
