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
    void procAsPlayedAlloc(float val)
    {
        if (val < 0.5)
            synth.voicePriorty = Motherboard::LOWEST;
        else
            synth.voicePriorty = Motherboard::LATEST;
        DBG("Set procAsPlayedAlloc voicePriority to " << (int)synth.voicePriorty);
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
    void setPolyphony(float param)
    {
        synth.setPolyphony(juce::roundToInt((param * (MAX_VOICES - 1)) + 1.f));
    }
    void setUnisonVoices(float param)
    {
        synth.setUnisonVoices(juce::roundToInt((param * (MAX_PANNINGS - 1)) + 1.f));
    }
    void procPitchBendUpRange(float param)
    {
        ForEachVoice(pitchWheelUpAmt = param * MAX_BEND_RANGE);
    }
    void procPitchBendDownRange(float param)
    {
        ForEachVoice(pitchWheelDownAmt = param * MAX_BEND_RANGE);
    }
    void procPitchWheelOsc2Only(float param) { ForEachVoice(pitchWheelOsc2Only = param > 0.5f); }
    void processPan(float param, int idx) { synth.pannings[(idx - 1) % MAX_PANNINGS] = param; }
    void processTune(float param) { ForEachVoice(osc.tune = param * 2.f - 1.f); }
    void processLegatoMode(float param)
    {
        ForEachVoice(legatoMode = juce::roundToInt(param * 3.f + 1.f) - 1);
    }
    void processOctave(float param)
    {
        ForEachVoice(osc.oct = juce::roundToInt(((param * 2.f) - 1.f) * 24.f));
    }
    void processFilterKeyFollow(float param) { ForEachVoice(fltKF = param); }
    void processSelfOscPush(float param)
    {
        ForEachVoice(selfOscPush = param > 0.5f);
        ForEachVoice(flt.selfOscPush = param > 0.5f);
    }
    void processXpanderFilter(float param) { ForEachVoice(flt.xpander = param > 0.5f); }
    void processXpanderMode(float param)
    {
        ForEachVoice(flt.xpanderMode = static_cast<uint8_t>(param * 15.f));
    }
    void processUnison(float param) { synth.uni = param > 0.5f; }
    void processPortamento(float param)
    {
        ForEachVoice(porta = logsc(1.f - param, 0.14f, 250.f, 150.f));
    }
    void processVolume(float param) { synth.Volume = linsc(param, 0.f, 0.30f); }
    void processLfoFrequency(float param)
    {
        synth.mlfo.setRawParam(param);
        synth.mlfo.setFrequency(logsc(param, 0.f, 250.f, 3775.f));
    }
    void processLfoSine(float param) { synth.mlfo.wave1blend = linsc(param, -1.f, 1.f); }
    void processLfoSquare(float param) { synth.mlfo.wave2blend = linsc(param, -1.f, 1.f); }
    void processLfoSH(float param) { synth.mlfo.wave3blend = linsc(param, -1.f, 1.f); }
    void processLfoPulsewidth(float param) { synth.mlfo.pw = param; }
    void processLfoAmt1(float param)
    {
        ForEachVoice(lfoa1 = logsc(logsc(param, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f));
    }
    void processLfoOsc1(float param) { ForEachVoice(lfoo1 = param > 0.5f); }
    void processLfoOsc2(float param) { ForEachVoice(lfoo2 = param > 0.5f); }
    void processLfoFilter(float param) { ForEachVoice(lfof = param > 0.5f); }
    void processLfoPw1(float param) { ForEachVoice(lfopw1 = param > 0.5f); }
    void processLfoPw2(float param) { ForEachVoice(lfopw2 = param > 0.5f); }
    void processLfoVolume(float param) { ForEachVoice(lfovol = param > 0.5f); }
    void processLfoAmt2(float param) { ForEachVoice(lfoa2 = linsc(param, 0.f, 0.7f)); }
    void processDetune(float param) { ForEachVoice(osc.totalDetune = logsc(param, 0.001f, 1.f)); }
    void processPulseWidth(float param) { ForEachVoice(osc.pulseWidth = linsc(param, 0.f, 0.95f)); }
    // PW env range is actually 0.95f, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 0.95f by a reciprocal of 0.9 here
    void processPwEnv(float param)
    {
        ForEachVoice(pwenvmod = linsc(param, 0.f, 1.055555555555555f));
    }
    void processPwOfs(float param) { ForEachVoice(pwOfs = linsc(param, 0.f, 0.95f)); }
    void processPwEnvBoth(float param) { ForEachVoice(pwEnvBoth = param > 0.5f); }
    void processInvertFenv(float param) { ForEachVoice(invertFenv = param > 0.5f); }
    void processPitchModBoth(float param) { ForEachVoice(pitchModBoth = param > 0.5f); }
    void processOsc2Xmod(float param) { ForEachVoice(osc.xmod = (param * 48.f)); }
    // pitch env range is actually 36 st, but because Envelope.h sustains at 90% fullscale
    // for some reason, we adjust 36 semitones by a reciprocal of 0.9 here
    void processEnvelopeToPitch(float param) { ForEachVoice(envpitchmod = (param * 40.f)); }
    void processOsc2HardSync(float param) { ForEachVoice(osc.hardSync = param > 0.5f); }
    void processOsc1Pitch(float param) { ForEachVoice(osc.osc1p = (param * 48.f)); }
    void processOsc2Pitch(float param) { ForEachVoice(osc.osc2p = (param * 48.f)); }
    void processEnvelopeToPitchInvert(float param) { ForEachVoice(osc.penvinv = param > 0.5f); }
    void processEnvelopeToPWInvert(float param) { ForEachVoice(osc.pwenvinv = param > 0.5f); }
    void processOsc1Mix(float param) { ForEachVoice(osc.osc1Mix = param); }
    void processOsc2Mix(float param) { ForEachVoice(osc.osc2Mix = param); }
    void processRingModMix(float param) { ForEachVoice(osc.ringModMix = param); }
    void processNoiseMix(float param) { ForEachVoice(osc.noiseMix = param); }
    void processNoiseColor(float param) { ForEachVoice(osc.noiseColor = param > 0.5f); }
    void processBrightness(float param)
    {
        ForEachVoice(setBrightness(linsc(param, 7000.f, 26000.f)));
    }
    void processOsc2Det(float param) { ForEachVoice(osc.osc2Det = logsc(param, 0.001f, 0.6f)); }
    void processOsc1Saw(float param) { ForEachVoice(osc.osc1Saw = param > 0.5f); }
    void processOsc1Pulse(float param) { ForEachVoice(osc.osc1Pul = param > 0.5f); }
    void processOsc2Saw(float param) { ForEachVoice(osc.osc2Saw = param > 0.5f); }
    void processOsc2Pulse(float param) { ForEachVoice(osc.osc2Pul = param > 0.5f); }
    void processCutoff(float param) { cutoffSmoother.setStep(linsc(param, 0.f, 120.f)); }
    inline void processCutoffSmoothed(float param) { ForEachVoice(cutoff = param); }
    void processResonance(float param)
    {
        resSmoother.setStep(0.991f - logsc(1.f - param, 0.f, 0.991f, 40.f));
    }
    inline void processResonanceSmoothed(float param)
    {
        ForEachVoice(flt.setResonance(0.991f - logsc(1.f - param, 0.f, 0.991f, 40.f)));
    }
    void processBandpassSw(float param) { ForEachVoice(flt.bandPassSw = param > 0.5f); }
    void processFourPole(float param) { ForEachVoice(fourpole = param > 0.5f); }
    void processMultimode(float param) { multimodeSmoother.setStep(param); }
    inline void processMultimodeSmoothed(float param) { ForEachVoice(flt.setMultimode(param)); }
    void processOversampling(float param) { synth.SetOversample(param > 0.5f); }
    void processFilterEnvelopeAmt(float param) { ForEachVoice(fenvamt = linsc(param, 0.f, 140.f)); }
    void processLoudnessEnvelopeAttack(float param)
    {
        ForEachVoice(env.setAttack(logsc(param, 4.f, 60000.f, 900.f)));
    }
    void processLoudnessEnvelopeDecay(float param)
    {
        ForEachVoice(env.setDecay(logsc(param, 4.f, 60000.f, 900.f)));
    }
    void processLoudnessEnvelopeSustain(float param) { ForEachVoice(env.setSustain(param)); }
    void processLoudnessEnvelopeRelease(float param)
    {
        ForEachVoice(env.setRelease(logsc(param, 8.f, 60000.f, 900.f)));
    }
    void processFilterEnvelopeAttack(float param)
    {
        ForEachVoice(fenv.setAttack(logsc(param, 1.f, 60000.f, 900.f)));
    }
    void processFilterEnvelopeDecay(float param)
    {
        ForEachVoice(fenv.setDecay(logsc(param, 1.f, 60000.f, 900.f)));
    }
    void processFilterEnvelopeSustain(float param) { ForEachVoice(fenv.setSustain(param)); }
    void processFilterEnvelopeRelease(float param)
    {
        ForEachVoice(fenv.setRelease(logsc(param, 1.f, 60000.f, 900.f)));
    }
    void processEnvelopeSlop(float param)
    {
        ForEachVoice(setEnvTimingOffset(linsc(param, 0.f, 1.f)));
    }
    void processFilterSlop(float param) { ForEachVoice(FltSlopAmt = linsc(param, 0.f, 18.f)); }
    void processPortamentoSlop(float param)
    {
        ForEachVoice(PortaSlopAmt = linsc(param, 0.f, 0.75f));
    }
    void processLoudnessSlop(float param) { ForEachVoice(levelSlopAmt = linsc(param, 0.f, 0.67f)); }
};

#endif // OBXF_SRC_ENGINE_SYNTHENGINE_H
