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

#include "ObxfVoice.h"
#include "Motherboard.h"
#include "Params.h"
#include "ParamSmoother.h"

class SynthEngine
{
  private:
    Motherboard synth;
    ParamSmoother cutoffSmoother;
    ParamSmoother pitchWheelSmoother;
    ParamSmoother modWheelSmoother;
    float sampleRate;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)

  public:
    SynthEngine() : cutoffSmoother(), pitchWheelSmoother(), modWheelSmoother() {}
    ~SynthEngine() {}

    void setPlayHead(float bpm, float retrPos) { synth.mlfo.hostSyncRetrigger(bpm, retrPos); }
    void setSampleRate(float sr)
    {
        sampleRate = sr;
        cutoffSmoother.setSampleRate(sr);
        pitchWheelSmoother.setSampleRate(sr);
        modWheelSmoother.setSampleRate(sr);
        synth.setSampleRate(sr);
    }

    void processSample(float *left, float *right)
    {
        processCutoffSmoothed(cutoffSmoother.smoothStep());
        procPitchWheelSmoothed(pitchWheelSmoother.smoothStep());
        procModWheelSmoothed(modWheelSmoother.smoothStep());

        synth.processSample(left, right);
    }

    void allNotesOff()
    {
        for (int i = 0; i < 128; i++)
        {
            procNoteOff(i);
        }
    }

#define ForEachVoice(expr)                                                                         \
    for (int i = 0; i < synth.MAX_VOICES; i++)                                                     \
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
    void procAsPlayedAlloc(float val) { synth.asPlayedMode = val > 0.5f; }
    void procNoteOn(int noteNo, float velocity) { synth.setNoteOn(noteNo, velocity); }
    void procNoteOff(int noteNo) { synth.setNoteOff(noteNo); }
    void procEconomyMode(float val) { synth.economyMode = val > 0.5f; }
    void procAmpVelocityAmount(float val) { ForEachVoice(vamp = val); }
    void procFltVelocityAmount(float val) { ForEachVoice(vflt = val); }
    void procModWheel(float val) { modWheelSmoother.setStep(val); }
    void procModWheelSmoothed(float val) { synth.vibratoAmount = val; }
    void procModWheelFrequency(float val)
    {
        synth.vibratoLfo.setFrequency(logsc(val, 3.f, 10.f));
        synth.vibratoEnabled = val > 0.05f;
    }
    void procPitchWheel(float val) { pitchWheelSmoother.setStep(val); }
    inline void procPitchWheelSmoothed(float val) { ForEachVoice(pitchWheel = val); }
    void setVoiceCount(float param)
    {
        synth.setVoiceCount(juce::roundToInt((param * (synth.MAX_VOICES - 1)) + 1.f));
    }
    void procPitchWheelAmount(float param)
    {
        ForEachVoice(pitchWheelAmt = param > 0.5f ? 12.f : 2.f);
    }
    void procPitchWheelOsc2Only(float param) { ForEachVoice(pitchWheelOsc2Only = param > 0.5f); }
    void processPan(float param, int idx)
    {
        synth.pannings[(idx - 1) % synth.MAX_PANNINGS] = param;
    }
    void processTune(float param) { ForEachVoice(osc.tune = param * 2.f - 1.f); }
    void processLegatoMode(float param)
    {
        ForEachVoice(legatoMode = juce::roundToInt(param * 3.f + 1.f) - 1);
    }
    void processOctave(float param)
    {
        ForEachVoice(osc.oct = (juce::roundToInt(param * 4.f) - 2) * 12);
    }
    void processFilterKeyFollow(float param) { ForEachVoice(fltKF = param); }
    void processSelfOscPush(float param)
    {
        ForEachVoice(selfOscPush = param > 0.5f);
        ForEachVoice(flt.selfOscPush = param > 0.5f);
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
        synth.mlfo.setFrequency(logsc(param, 0.f, 50.f, 120.f));
    }
    void processLfoSine(float param)
    {
        if (param > 0.5f)
        {
            synth.mlfo.waveForm |= 1;
        }
        else
        {
            synth.mlfo.waveForm &= ~1;
        }
    }
    void processLfoSquare(float param)
    {
        if (param > 0.5f)
        {
            synth.mlfo.waveForm |= 2;
        }
        else
        {
            synth.mlfo.waveForm &= ~2;
        }
    }
    void processLfoSH(float param)
    {
        if (param > 0.5f)
        {
            synth.mlfo.waveForm |= 4;
        }
        else
        {
            synth.mlfo.waveForm &= ~4;
        }
    }
    void processLfoAmt1(float param)
    {
        ForEachVoice(lfoa1 = logsc(logsc(param, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f));
    }
    void processLfoOsc1(float param) { ForEachVoice(lfoo1 = param > 0.5f); }
    void processLfoOsc2(float param) { ForEachVoice(lfoo2 = param > 0.5f); }
    void processLfoFilter(float param) { ForEachVoice(lfof = param > 0.5f); }
    void processLfoPw1(float param) { ForEachVoice(lfopw1 = param > 0.5f); }
    void processLfoPw2(float param) { ForEachVoice(lfopw2 = param > 0.5f); }
    void processLfoAmt2(float param) { ForEachVoice(lfoa2 = linsc(param, 0.f, 0.7f)); }
    void processDetune(float param) { ForEachVoice(osc.totalDetune = logsc(param, 0.001f, 0.9f)); }
    void processPulseWidth(float param) { ForEachVoice(osc.pulseWidth = linsc(param, 0.f, 0.95f)); }
    void processPwEnv(float param) { ForEachVoice(pwenvmod = linsc(param, 0.f, 0.85f)); }
    void processPwOfs(float param) { ForEachVoice(pwOfs = linsc(param, 0.f, 0.75f)); }
    void processPwEnvBoth(float param) { ForEachVoice(pwEnvBoth = param > 0.5f); }
    void processInvertFenv(float param) { ForEachVoice(invertFenv = param > 0.5f); }
    void processPitchModBoth(float param) { ForEachVoice(pitchModBoth = param > 0.5f); }
    void processOsc2Xmod(float param) { ForEachVoice(osc.xmod = (param * 24.f)); }
    void processEnvelopeToPitch(float param) { ForEachVoice(envpitchmod = (param * 36.f)); }
    void processOsc2HardSync(float param) { ForEachVoice(osc.hardSync = param > 0.5f); }
    void processOsc1Pitch(float param) { ForEachVoice(osc.osc1p = (param * 48.f)); }
    void processOsc2Pitch(float param) { ForEachVoice(osc.osc2p = (param * 48.f)); }
    void processPitchQuantization(float param) { ForEachVoice(osc.quantizeCw = param > 0.5f); }
    void processOsc1Mix(float param) { ForEachVoice(osc.o1mx = param); }
    void processOsc2Mix(float param) { ForEachVoice(osc.o2mx = param); }
    void processNoiseMix(float param) { ForEachVoice(osc.nmx = logsc(param, 0.f, 1.f, 35.f)); }
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
    void processBandpassSw(float param) { ForEachVoice(flt.bandPassSw = param > 0.5f); }
    void processResonance(float param)
    {
        ForEachVoice(flt.setResonance(0.991f - logsc(1.f - param, 0.f, 0.991f, 40.f)));
    }
    void processFourPole(float param) { ForEachVoice(fourpole = param > 0.5f); }
    void processMultimode(float param) { ForEachVoice(flt.setMultimode(linsc(param, 0.f, 1.f))); }
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
