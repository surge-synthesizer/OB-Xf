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
            {Filter2PolePush, [](SynthEngine &s, float v) { s.processSelfOscPush(v); }},
            {Filter4PoleXpander, [](SynthEngine &s, float v) { s.processXpanderFilter(v); }},
            {FilterXpanderMode, [](SynthEngine &s, float v) { s.processXpanderMode(v); }},
            {EnvToPWBothOscs, [](SynthEngine &s, float v) { s.processPwEnvBoth(v); }},
            {Osc2PWOffset, [](SynthEngine &s, float v) { s.processPwOfs(v); }},
            {EnvToPitchBothOscs, [](SynthEngine &s, float v) { s.processPitchModBoth(v); }},
            {FilterEnvInvert, [](SynthEngine &s, float v) { s.processInvertFenv(v); }},
            {LevelSlop, [](SynthEngine &s, float v) { s.processLoudnessSlop(v); }},
            {EnvToPW, [](SynthEngine &s, float v) { s.processPwEnv(v); }},
            {Lfo1TempoSync, [](SynthEngine &s, float v) { s.procLfoSync(v); }},
            {EcoMode, [](SynthEngine &s, float v) { s.procEconomyMode(v); }},
            {VelToAmpEnv, [](SynthEngine &s, float v) { s.procAmpVelocityAmount(v); }},
            {VelToFilterEnv, [](SynthEngine &s, float v) { s.procFltVelocityAmount(v); }},
            {NotePriority, [](SynthEngine &s, float v) { s.procNotePriority(v); }},
            {VibratoRate, [](SynthEngine &s, float v) { s.procModWheelFrequency(v); }},
            {VibratoWave, [](SynthEngine &s, float v) { s.procModWheelWave(v); }},
            {Filter4PoleMode, [](SynthEngine &s, float v) { s.processFourPole(v); }},
            {EnvLegatoMode, [](SynthEngine &s, float v) { s.processLegatoMode(v); }},
            {EnvToPitch, [](SynthEngine &s, float v) { s.processEnvelopeToPitch(v); }},
            {EnvToPitchInvert, [](SynthEngine &s, float v) { s.processEnvelopeToPitchInvert(v); }},
            {EnvToPWInvert, [](SynthEngine &s, float v) { s.processEnvelopeToPWInvert(v); }},
            {Polyphony, [](SynthEngine &s, float v) { s.setPolyphony(v); }},
            {UnisonVoices, [](SynthEngine &s, float v) { s.setUnisonVoices(v); }},
            {Filter2PoleBPBlend, [](SynthEngine &s, float v) { s.processBandpassSw(v); }},
            {HQMode, [](SynthEngine &s, float v) { s.processOversampling(v); }},
            {PitchBendOsc2Only, [](SynthEngine &s, float v) { s.procPitchWheelOsc2Only(v); }},
            {PitchBendUpRange, [](SynthEngine &s, float v) { s.procPitchBendUpRange(v); }},
            {PitchBendDownRange, [](SynthEngine &s, float v) { s.procPitchBendDownRange(v); }},
            {RingModMix, [](SynthEngine &s, float v) { s.processRingModMix(v); }},
            {NoiseMix, [](SynthEngine &s, float v) { s.processNoiseMix(v); }},
            {NoiseColor, [](SynthEngine &s, float v) { s.processNoiseColor(v); }},
            {Transpose, [](SynthEngine &s, float v) { s.processOctave(v); }},
            {Tune, [](SynthEngine &s, float v) { s.processTune(v); }},
            {Brightness, [](SynthEngine &s, float v) { s.processBrightness(v); }},
            {FilterMode, [](SynthEngine &s, float v) { s.processMultimode(v); }},
            {Lfo1Rate, [](SynthEngine &s, float v) { s.processLfoFrequency(v); }},
            {LfoModAmt1, [](SynthEngine &s, float v) { s.processLfoAmt1(v); }},
            {LfoModAmt2, [](SynthEngine &s, float v) { s.processLfoAmt2(v); }},
            {Lfo1Wave1, [](SynthEngine &s, float v) { s.processLfoSine(v); }},
            {Lfo1Wave2, [](SynthEngine &s, float v) { s.processLfoSquare(v); }},
            {Lfo1Wave3, [](SynthEngine &s, float v) { s.processLfoSH(v); }},
            {LfoToFilterCutoff, [](SynthEngine &s, float v) { s.processLfoFilter(v); }},
            {LfoToOsc1Pitch, [](SynthEngine &s, float v) { s.processLfoOsc1(v); }},
            {LfoToOsc2Pitch, [](SynthEngine &s, float v) { s.processLfoOsc2(v); }},
            {LfoToOsc1PW, [](SynthEngine &s, float v) { s.processLfoPw1(v); }},
            {LfoToOsc2PW, [](SynthEngine &s, float v) { s.processLfoPw2(v); }},
            {LfoToVolume, [](SynthEngine &s, float v) { s.processLfoVolume(v); }},
            {LfoPulseWidth, [](SynthEngine &s, float v) { s.processLfoPulsewidth(v); }},
            {PortamentoSlop, [](SynthEngine &s, float v) { s.processPortamentoSlop(v); }},
            {FilterSlop, [](SynthEngine &s, float v) { s.processFilterSlop(v); }},
            {EnvelopeSlop, [](SynthEngine &s, float v) { s.processEnvelopeSlop(v); }},
            {Crossmod, [](SynthEngine &s, float v) { s.processOsc2Xmod(v); }},
            {OscSync, [](SynthEngine &s, float v) { s.processOsc2HardSync(v); }},
            {Osc2Pitch, [](SynthEngine &s, float v) { s.processOsc2Pitch(v); }},
            {Osc1Pitch, [](SynthEngine &s, float v) { s.processOsc1Pitch(v); }},
            {Portamento, [](SynthEngine &s, float v) { s.processPortamento(v); }},
            {Unison, [](SynthEngine &s, float v) { s.processUnison(v); }},
            {FilterKeyFollow, [](SynthEngine &s, float v) { s.processFilterKeyFollow(v); }},
            {Osc1Mix, [](SynthEngine &s, float v) { s.processOsc1Mix(v); }},
            {Osc2Mix, [](SynthEngine &s, float v) { s.processOsc2Mix(v); }},
            {OscPulseWidth, [](SynthEngine &s, float v) { s.processPulseWidth(v); }},
            {Osc1SawWave, [](SynthEngine &s, float v) { s.processOsc1Saw(v); }},
            {Osc2SawWave, [](SynthEngine &s, float v) { s.processOsc2Saw(v); }},
            {Osc1PulseWave, [](SynthEngine &s, float v) { s.processOsc1Pulse(v); }},
            {Osc2PulseWave, [](SynthEngine &s, float v) { s.processOsc2Pulse(v); }},
            {Volume, [](SynthEngine &s, float v) { s.processVolume(v); }},
            {UnisonDetune, [](SynthEngine &s, float v) { s.processDetune(v); }},
            {Osc2Detune, [](SynthEngine &s, float v) { s.processOsc2Det(v); }},
            {FilterCutoff, [](SynthEngine &s, float v) { s.processCutoff(v); }},
            {FilterResonance, [](SynthEngine &s, float v) { s.processResonance(v); }},
            {FilterEnvAmount, [](SynthEngine &s, float v) { s.processFilterEnvelopeAmt(v); }},
            {AmpEnvAttack, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeAttack(v); }},
            {AmpEnvDecay, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeDecay(v); }},
            {AmpEnvSustain, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeSustain(v); }},
            {AmpEnvRelease, [](SynthEngine &s, float v) { s.processLoudnessEnvelopeRelease(v); }},
            {FilterEnvAttack, [](SynthEngine &s, float v) { s.processFilterEnvelopeAttack(v); }},
            {FilterEnvDecay, [](SynthEngine &s, float v) { s.processFilterEnvelopeDecay(v); }},
            {FilterEnvSustain, [](SynthEngine &s, float v) { s.processFilterEnvelopeSustain(v); }},
            {FilterEnvRelease, [](SynthEngine &s, float v) { s.processFilterEnvelopeRelease(v); }},
            {PanVoice1, [](SynthEngine &s, float v) { s.processPan(v, 1); }},
            {PanVoice2, [](SynthEngine &s, float v) { s.processPan(v, 2); }},
            {PanVoice3, [](SynthEngine &s, float v) { s.processPan(v, 3); }},
            {PanVoice4, [](SynthEngine &s, float v) { s.processPan(v, 4); }},
            {PanVoice5, [](SynthEngine &s, float v) { s.processPan(v, 5); }},
            {PanVoice6, [](SynthEngine &s, float v) { s.processPan(v, 6); }},
            {PanVoice7, [](SynthEngine &s, float v) { s.processPan(v, 7); }},
            {PanVoice8, [](SynthEngine &s, float v) { s.processPan(v, 8); }},
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
