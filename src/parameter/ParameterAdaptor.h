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

#ifndef OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H
#define OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H

#include <juce_audio_basics/juce_audio_basics.h>
#include "engine/SynthEngine.h"

#include "IParameterState.h"
#include "IProgramState.h"
#include "SynthParam.h"
#include "ParameterManager.h"
#include "ValueAttachment.h"
#include "ParameterList.h"

class ParameterManagerAdaptor
{
  public:
    ValueAttachment<bool> midiLearnAttachment{};
    ValueAttachment<bool> midiUnlearnAttachment{};

    ParameterManagerAdaptor(IParameterState &paramState, IProgramState &progState,
                            juce::AudioProcessor &processor)
        : parameterState(paramState), programState(progState),
          paramManager(processor, "SynthParams", ParameterList)
    {
        setupParameterCallbacks();
    }

    void setEngineParameterValue(SynthEngine &synth, const juce::String &paramId, float newValue,
                                 bool notifyToHost = false)
    {
        if (!parameterState.getMidiControlledParamSet())
            parameterState.setLastUsedParameter(paramId);

        auto *param = paramManager.getAPVTS().getParameter(paramId);
        if (param == nullptr)
            return;

        // TODO: Dont think needed, but left here for reference
        //  float normalizedValue = newValue;
        //  for (const auto &paramInfo : paramManager.getParameters())
        //  {
        //      if (paramInfo.ID == paramId)
        //      {
        //          normalizedValue =
        //              juce::jmap(newValue, paramInfo.meta.minVal, paramInfo.meta.maxVal,
        //              0.0f, 1.0f);
        //          break;
        //      }
        //  }

        programState.updateProgramValue(paramId, newValue);

        if (engine != &synth)
        {
            setEngine(synth);
        }

        if (notifyToHost)
            param->setValueNotifyingHost(newValue);
        else
            param->setValue(newValue);

        if (parameterState.getIsHostAutomatedChange())
            parameterState.sendChangeMessage();
    }

    juce::AudioProcessorValueTreeState &getValueTreeState() { return paramManager.getAPVTS(); }

    void updateParameters(bool force = false) { paramManager.updateParameters(force); }

    void setEngine(SynthEngine &synth)
    {
        engine = &synth;
        setupParameterCallbacks();
    }

    SynthEngine &getEngine() const { return *engine; }

    void flushFIFO() { paramManager.flushParameterQueue(); }

    void clearFIFO() { paramManager.clearFiFO(); }

  private:
    void setupParameterCallbacks()
    {
        for (const auto &paramInfo : ParameterList)
        {
            const juce::String &paramId = paramInfo.ID;
            paramManager.registerParameterCallback(
                paramId, [this, paramId](const float newValue, bool /*forced*/) {
                    if (this->engine)
                    {
                        processParameterChange(*this->engine, paramId, newValue);
                        this->programState.updateProgramValue(paramId, newValue);
                    }
                });
        }
    }

    void processParameterChange(SynthEngine &synth, const juce::String &paramId,
                                const float newValue)
    {
        using Handler = std::function<void(SynthEngine &, float)>;
        using namespace SynthParam::ID;

        static const std::unordered_map<std::string, Handler> handlers = {
            {SelfOscPush, [](SynthEngine &s, float v) { s.processSelfOscPush(v); }},
            {XpanderFilter, [](SynthEngine &s, float v) { s.processXpanderFilter(v); }},
            {XpanderMode, [](SynthEngine &s, float v) { s.processXpanderMode(v); }},
            {PwEnvBoth, [](SynthEngine &s, float v) { s.processPwEnvBoth(v); }},
            {PwOsc2Ofs, [](SynthEngine &s, float v) { s.processPwOfs(v); }},
            {EnvPitchBoth, [](SynthEngine &s, float v) { s.processPitchModBoth(v); }},
            {FenvInvert, [](SynthEngine &s, float v) { s.processInvertFenv(v); }},
            {LevelDetune, [](SynthEngine &s, float v) { s.processLoudnessSlop(v); }},
            {PwEnv, [](SynthEngine &s, float v) { s.processPwEnv(v); }},
            {LfoSync, [](SynthEngine &s, float v) { s.procLfoSync(v); }},
            {EconomyMode, [](SynthEngine &s, float v) { s.procEconomyMode(v); }},
            {VAmpFactor, [](SynthEngine &s, float v) { s.procAmpVelocityAmount(v); }},
            {VFltFactor, [](SynthEngine &s, float v) { s.procFltVelocityAmount(v); }},
            {NotePriority, [](SynthEngine &s, float v) { s.procNotePriority(v); }},
            {VibratoRate, [](SynthEngine &s, float v) { s.procModWheelFrequency(v); }},
            {VibratoWave, [](SynthEngine &s, float v) { s.procModWheelWave(v); }},
            {FourPole, [](SynthEngine &s, float v) { s.processFourPole(v); }},
            {EnvLegatoMode, [](SynthEngine &s, float v) { s.processLegatoMode(v); }},
            {EnvelopeToPitch, [](SynthEngine &s, float v) { s.processEnvelopeToPitch(v); }},
            {EnvelopeToPitchInv,
             [](SynthEngine &s, float v) { s.processEnvelopeToPitchInvert(v); }},
            {EnvelopeToPWInv, [](SynthEngine &s, float v) { s.processEnvelopeToPWInvert(v); }},
            {Polyphony, [](SynthEngine &s, float v) { s.setPolyphony(v); }},
            {UnisonVoices, [](SynthEngine &s, float v) { s.setUnisonVoices(v); }},
            {BandpassBlend, [](SynthEngine &s, float v) { s.processBandpassSw(v); }},
            {HQMode, [](SynthEngine &s, float v) { s.processOversampling(v); }},
            {BendOsc2Only, [](SynthEngine &s, float v) { s.procPitchWheelOsc2Only(v); }},
            {PitchBendUpRange, [](SynthEngine &s, float v) { s.procPitchBendUpRange(v); }},
            {PitchBendDownRange, [](SynthEngine &s, float v) { s.procPitchBendDownRange(v); }},
            {RingModMix, [](SynthEngine &s, float v) { s.processRingModMix(v); }},
            {NoiseMix, [](SynthEngine &s, float v) { s.processNoiseMix(v); }},
            {NoiseColor, [](SynthEngine &s, float v) { s.processNoiseColor(v); }},
            {Transpose, [](SynthEngine &s, float v) { s.processOctave(v); }},
            {Tune, [](SynthEngine &s, float v) { s.processTune(v); }},
            {Brightness, [](SynthEngine &s, float v) { s.processBrightness(v); }},
            {Multimode, [](SynthEngine &s, float v) { s.processMultimode(v); }},
            {LfoFrequency, [](SynthEngine &s, float v) { s.processLfoFrequency(v); }},
            {LfoAmount1, [](SynthEngine &s, float v) { s.processLfoAmt1(v); }},
            {LfoAmount2, [](SynthEngine &s, float v) { s.processLfoAmt2(v); }},
            {LfoSineWave, [](SynthEngine &s, float v) { s.processLfoSine(v); }},
            {LfoSquareWave, [](SynthEngine &s, float v) { s.processLfoSquare(v); }},
            {LfoSampleHoldWave, [](SynthEngine &s, float v) { s.processLfoSH(v); }},
            {LfoFilter, [](SynthEngine &s, float v) { s.processLfoFilter(v); }},
            {LfoOsc1, [](SynthEngine &s, float v) { s.processLfoOsc1(v); }},
            {LfoOsc2, [](SynthEngine &s, float v) { s.processLfoOsc2(v); }},
            {LfoPw1, [](SynthEngine &s, float v) { s.processLfoPw1(v); }},
            {LfoPw2, [](SynthEngine &s, float v) { s.processLfoPw2(v); }},
            {LfoVolume, [](SynthEngine &s, float v) { s.processLfoVolume(v); }},
            {LfoPulsewidth, [](SynthEngine &s, float v) { s.processLfoPulsewidth(v); }},
            {PortamentoDetune, [](SynthEngine &s, float v) { s.processPortamentoSlop(v); }},
            {FilterDetune, [](SynthEngine &s, float v) { s.processFilterSlop(v); }},
            {EnvelopeDetune, [](SynthEngine &s, float v) { s.processEnvelopeSlop(v); }},
            {Xmod, [](SynthEngine &s, float v) { s.processOsc2Xmod(v); }},
            {Osc2HardSync, [](SynthEngine &s, float v) { s.processOsc2HardSync(v); }},
            {Osc2Pitch, [](SynthEngine &s, float v) { s.processOsc2Pitch(v); }},
            {Osc1Pitch, [](SynthEngine &s, float v) { s.processOsc1Pitch(v); }},
            {Portamento, [](SynthEngine &s, float v) { s.processPortamento(v); }},
            {Unison, [](SynthEngine &s, float v) { s.processUnison(v); }},
            {FilterKeyFollow, [](SynthEngine &s, float v) { s.processFilterKeyFollow(v); }},
            {Osc1Mix, [](SynthEngine &s, float v) { s.processOsc1Mix(v); }},
            {Osc2Mix, [](SynthEngine &s, float v) { s.processOsc2Mix(v); }},
            {PulseWidth, [](SynthEngine &s, float v) { s.processPulseWidth(v); }},
            {Osc1Saw, [](SynthEngine &s, float v) { s.processOsc1Saw(v); }},
            {Osc2Saw, [](SynthEngine &s, float v) { s.processOsc2Saw(v); }},
            {Osc1Pulse, [](SynthEngine &s, float v) { s.processOsc1Pulse(v); }},
            {Osc2Pulse, [](SynthEngine &s, float v) { s.processOsc2Pulse(v); }},
            {Volume, [](SynthEngine &s, float v) { s.processVolume(v); }},
            {VoiceDetune, [](SynthEngine &s, float v) { s.processDetune(v); }},
            {Oscillator2Detune, [](SynthEngine &s, float v) { s.processOsc2Det(v); }},
            {Cutoff, [](SynthEngine &s, float v) { s.processCutoff(v); }},
            {Resonance, [](SynthEngine &s, float v) { s.processResonance(v); }},
            {FilterEnvAmount, [](SynthEngine &s, float v) { s.processFilterEnvelopeAmt(v); }},
            {Attack, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeAttack(v); }},
            {Decay, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeDecay(v); }},
            {Sustain, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeSustain(v); }},
            {Release, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeRelease(v); }},
            {FilterAttack, [](SynthEngine &s, float v) { s.processFilterEnvelopeAttack(v); }},
            {FilterDecay, [](SynthEngine &s, float v) { s.processFilterEnvelopeDecay(v); }},
            {FilterSustain, [](SynthEngine &s, float v) { s.processFilterEnvelopeSustain(v); }},
            {FilterRelease, [](SynthEngine &s, float v) { s.processFilterEnvelopeRelease(v); }},
            {Pan1, [](SynthEngine &s, float v) { s.processPan(v, 1); }},
            {Pan2, [](SynthEngine &s, float v) { s.processPan(v, 2); }},
            {Pan3, [](SynthEngine &s, float v) { s.processPan(v, 3); }},
            {Pan4, [](SynthEngine &s, float v) { s.processPan(v, 4); }},
            {Pan5, [](SynthEngine &s, float v) { s.processPan(v, 5); }},
            {Pan6, [](SynthEngine &s, float v) { s.processPan(v, 6); }},
            {Pan7, [](SynthEngine &s, float v) { s.processPan(v, 7); }},
            {Pan8, [](SynthEngine &s, float v) { s.processPan(v, 8); }},
        };

        const std::string idStr = paramId.toStdString();
        if (const auto it = handlers.find(idStr); it != handlers.end())
        {
            it->second(synth, newValue);
        }
    }

    IParameterState &parameterState;
    IProgramState &programState;
    ParameterManager paramManager;
    SynthEngine *engine = nullptr;
};

#endif // OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H
