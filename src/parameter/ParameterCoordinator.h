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

#ifndef OBXF_SRC_PARAMETER_PARAMETERCOORDINATOR_H
#define OBXF_SRC_PARAMETER_PARAMETERCOORDINATOR_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <random>
#include <set>
#include "engine/SynthEngine.h"

#include "IParameterState.h"
#include "IProgramState.h"
#include "SynthParam.h"
#include "ParameterUpdateHandler.h"
#include "ValueAttachment.h"
#include "ParameterList.h"

class ObxfAudioProcessor;

/**
 * The parameter coordinator is sort of the central hub for navigating between
 * the program the midi handler the editor the processor and the updater. It
 * sets up param->engine callbacks and holds the queue for updates onto params
 * and onto the engine and so forth. Basically its the hub so all the biuts talk
 * to each other.
 */
class ParameterCoordinator
{
  public:
    ValueAttachment<bool> midiLearnAttachment{};

    ParameterCoordinator(IParameterState &paramState, IProgramState &progState,
                            ObxfAudioProcessor &processor, SynthEngine &synth);

    void setEngineParameterValue(SynthEngine & /*synth*/, const juce::String &paramId,
                                 float newValue, bool notifyToHost = false)
    {
        if (!parameterState.getMidiLearnParameterSelected())
        {
            parameterState.setLastUsedParameter(paramId);
        }

        auto *param = updateHandler.getParameter(paramId);

        if (param == nullptr)
        {
            return;
        }

        programState.updateProgramValue(paramId, newValue);

        if (notifyToHost)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(newValue);
            param->endChangeGesture();
        }

        if (parameterState.getIsHostAutomatedChange())
        {
            parameterState.sendChangeMessage();
        }
    }

    ParameterUpdateHandler &getParameterUpdateHandler() { return updateHandler; }
    const ParameterUpdateHandler &getParameterUpdateHandler() const { return updateHandler; }

    // In an ideal world we wouldn't have this alias. But some battles arent worth it.
    juce::RangedAudioParameter *getParameter(const juce::String &paramID) const
    {
        return updateHandler.getParameter(paramID);
    }

  private:
    void setupParameterCallbacks()
    {
        for (const auto &paramInfo : ParameterList)
        {
            const juce::String &paramId = paramInfo.ID;
            auto hid = getHandlerMap().find(paramId.toStdString());
            if (hid == getHandlerMap().end())
            {
                OBLOG(params, "Unable to locate callback for " << paramId);
            }
            else
            {
                updateHandler.addParameterCallback(
                    paramId, "PROGRAM",
                    [cb = hid->second, this, paramId](const float newValue, bool /*forced*/) {
                        cb(engine, newValue);
                        this->programState.updateProgramValue(paramId, newValue);
                    });
            }
        }
    }

    using Handler = std::function<void(SynthEngine &, float)>;

    const std::unordered_map<std::string, Handler> &getHandlerMap()
    {
        using namespace SynthParam::ID;

        static const std::unordered_map<std::string, Handler> handlers = {
            {Filter2PolePush, [](SynthEngine &s, float v) { s.processFilter2PolePush(v); }},
            {Filter4PoleXpander, [](SynthEngine &s, float v) { s.processFilter4PoleXpander(v); }},
            {FilterXpanderMode, [](SynthEngine &s, float v) { s.processFilterXpanderMode(v); }},
            {EnvToPWBothOscs, [](SynthEngine &s, float v) { s.processEnvToPWBothOscs(v); }},
            {Osc2PWOffset, [](SynthEngine &s, float v) { s.processOsc2PWOffset(v); }},
            {EnvToPitchBothOscs, [](SynthEngine &s, float v) { s.processPitchBothOscs(v); }},
            {FilterEnvInvert, [](SynthEngine &s, float v) { s.processFilterEnvInvert(v); }},
            {LevelSlop, [](SynthEngine &s, float v) { s.processLevelSlop(v); }},
            {EnvToPWAmount, [](SynthEngine &s, float v) { s.processEnvToPWAmount(v); }},
            {LFO1TempoSync, [](SynthEngine &s, float v) { s.processLFO1Sync(v); }},
            {LFO2TempoSync, [](SynthEngine &s, float v) { s.processLFO2Sync(v); }},
            {VelToAmpEnv, [](SynthEngine &s, float v) { s.processVelToAmpEnv(v); }},
            {VelToFilterEnv, [](SynthEngine &s, float v) { s.processVelToFilterEnv(v); }},
            {NotePriority, [](SynthEngine &s, float v) { s.processNotePriority(v); }},
            {VibratoRate, [](SynthEngine &s, float v) { s.processVibratoLFORate(v); }},
            {VibratoWave, [](SynthEngine &s, float v) { s.processVibratoLFOWave(v); }},
            {Filter4PoleMode, [](SynthEngine &s, float v) { s.processFilter4PoleMode(v); }},
            {EnvLegatoMode, [](SynthEngine &s, float v) { s.processEnvLegatoMode(v); }},
            {EnvToPitchAmount, [](SynthEngine &s, float v) { s.processEnvToPitchAmount(v); }},
            {EnvToPitchInvert, [](SynthEngine &s, float v) { s.processEnvToPitchInvert(v); }},
            {EnvToPWInvert, [](SynthEngine &s, float v) { s.processEnvToPWInvert(v); }},
            {Polyphony, [](SynthEngine &s, float v) { s.processPolyphony(v); }},
            {UnisonVoices, [](SynthEngine &s, float v) { s.processUnisonVoices(v); }},
            {Filter2PoleBPBlend, [](SynthEngine &s, float v) { s.processFilter2PoleBPBlend(v); }},
            {HQMode, [](SynthEngine &s, float v) { s.processHQMode(v); }},
            {BendOsc2Only, [](SynthEngine &s, float v) { s.processBendOsc2Only(v); }},
            {BendUpRange, [](SynthEngine &s, float v) { s.processBendUpRange(v); }},
            {BendDownRange, [](SynthEngine &s, float v) { s.processBendDownRange(v); }},
            {RingModVol, [](SynthEngine &s, float v) { s.processRingModVolume(v); }},
            {NoiseVol, [](SynthEngine &s, float v) { s.processNoiseVolume(v); }},
            {NoiseColor, [](SynthEngine &s, float v) { s.processNoiseColor(v); }},
            {Transpose, [](SynthEngine &s, float v) { s.processTranspose(v); }},
            {Tune, [](SynthEngine &s, float v) { s.processTune(v); }},
            {OscBrightness, [](SynthEngine &s, float v) { s.processOscBrightness(v); }},
            {FilterMode, [](SynthEngine &s, float v) { s.processFilterMode(v); }},
            {LFO1Rate, [](SynthEngine &s, float v) { s.processLFO1Rate(v); }},
            {LFO1ModAmount1, [](SynthEngine &s, float v) { s.processLFO1ModAmount1(v); }},
            {LFO1ModAmount2, [](SynthEngine &s, float v) { s.processLFO1ModAmount2(v); }},
            {LFO1Wave1, [](SynthEngine &s, float v) { s.processLFO1Wave1(v); }},
            {LFO1Wave2, [](SynthEngine &s, float v) { s.processLFO1Wave2(v); }},
            {LFO1Wave3, [](SynthEngine &s, float v) { s.processLFO1Wave3(v); }},
            {LFO1ToFilterCutoff, [](SynthEngine &s, float v) { s.processLFO1ToFilterCutoff(v); }},
            {LFO1ToOsc1Pitch, [](SynthEngine &s, float v) { s.processLFO1ToOsc1Pitch(v); }},
            {LFO1ToOsc2Pitch, [](SynthEngine &s, float v) { s.processLFO1ToOsc2Pitch(v); }},
            {LFO1ToOsc1PW, [](SynthEngine &s, float v) { s.processLFO1ToOsc1PW(v); }},
            {LFO1ToOsc2PW, [](SynthEngine &s, float v) { s.processLFO1ToOsc2PW(v); }},
            {LFO1ToVolume, [](SynthEngine &s, float v) { s.processLFO1ToVolume(v); }},
            {LFO1PW, [](SynthEngine &s, float v) { s.processLFO1PW(v); }},
            {LFO2Rate, [](SynthEngine &s, float v) { s.processLFO2Rate(v); }},
            {LFO2ModAmount1, [](SynthEngine &s, float v) { s.processLFO2ModAmount1(v); }},
            {LFO2ModAmount2, [](SynthEngine &s, float v) { s.processLFO2ModAmount2(v); }},
            {LFO2Wave1, [](SynthEngine &s, float v) { s.processLFO2Wave1(v); }},
            {LFO2Wave2, [](SynthEngine &s, float v) { s.processLFO2Wave2(v); }},
            {LFO2Wave3, [](SynthEngine &s, float v) { s.processLFO2Wave3(v); }},
            {LFO2ToFilterCutoff, [](SynthEngine &s, float v) { s.processLFO2ToFilterCutoff(v); }},
            {LFO2ToOsc1Pitch, [](SynthEngine &s, float v) { s.processLFO2ToOsc1Pitch(v); }},
            {LFO2ToOsc2Pitch, [](SynthEngine &s, float v) { s.processLFO2ToOsc2Pitch(v); }},
            {LFO2ToOsc1PW, [](SynthEngine &s, float v) { s.processLFO2ToOsc1PW(v); }},
            {LFO2ToOsc2PW, [](SynthEngine &s, float v) { s.processLFO2ToOsc2PW(v); }},
            {LFO2ToVolume, [](SynthEngine &s, float v) { s.processLFO2ToVolume(v); }},
            {LFO2PW, [](SynthEngine &s, float v) { s.processLFO2PW(v); }},
            {PortamentoSlop, [](SynthEngine &s, float v) { s.processPortamentoSlop(v); }},
            {FilterSlop, [](SynthEngine &s, float v) { s.processFilterSlop(v); }},
            {EnvelopeSlop, [](SynthEngine &s, float v) { s.processEnvelopeSlop(v); }},
            {OscCrossmod, [](SynthEngine &s, float v) { s.processCrossmod(v); }},
            {OscSync, [](SynthEngine &s, float v) { s.processOscSync(v); }},
            {Osc2Pitch, [](SynthEngine &s, float v) { s.processOsc2Pitch(v); }},
            {Osc1Pitch, [](SynthEngine &s, float v) { s.processOsc1Pitch(v); }},
            {Portamento, [](SynthEngine &s, float v) { s.processPortamento(v); }},
            {Unison, [](SynthEngine &s, float v) { s.processUnison(v); }},
            {FilterKeyTrack, [](SynthEngine &s, float v) { s.processFilterKeyTrack(v); }},
            {Osc1Vol, [](SynthEngine &s, float v) { s.processOsc1Volume(v); }},
            {Osc2Vol, [](SynthEngine &s, float v) { s.processOsc2Volume(v); }},
            {OscPW, [](SynthEngine &s, float v) { s.processOscPW(v); }},
            {Osc1SawWave, [](SynthEngine &s, float v) { s.processOsc1Saw(v); }},
            {Osc2SawWave, [](SynthEngine &s, float v) { s.processOsc2Saw(v); }},
            {Osc1PulseWave, [](SynthEngine &s, float v) { s.processOsc1Pulse(v); }},
            {Osc2PulseWave, [](SynthEngine &s, float v) { s.processOsc2Pulse(v); }},
            {Volume, [](SynthEngine &s, float v) { s.processVolume(v); }},
            {UnisonDetune, [](SynthEngine &s, float v) { s.processUnisonDetune(v); }},
            {Osc2Detune, [](SynthEngine &s, float v) { s.processOsc2Detune(v); }},
            {FilterCutoff, [](SynthEngine &s, float v) { s.processFilterCutoff(v); }},
            {FilterResonance, [](SynthEngine &s, float v) { s.processFilterResonance(v); }},
            {FilterEnvAmount, [](SynthEngine &s, float v) { s.processFilterEnvAmount(v); }},
            {AmpEnvAttackCurve, [](SynthEngine &s, float v) { s.processAmpEnvAttackCurve(v); }},
            {AmpEnvAttack, [](SynthEngine &s, float v) { s.processAmpEnvAttack(v); }},
            {AmpEnvDecay, [](SynthEngine &s, float v) { s.processAmpEnvDecay(v); }},
            {AmpEnvSustain, [](SynthEngine &s, float v) { s.processAmpEnvSustain(v); }},
            {AmpEnvRelease, [](SynthEngine &s, float v) { s.processAmpEnvRelease(v); }},
            {FilterEnvAttackCurve,
             [](SynthEngine &s, float v) { s.processFilterEnvAttackCurve(v); }},
            {FilterEnvAttack, [](SynthEngine &s, float v) { s.processFilterEnvAttack(v); }},
            {FilterEnvDecay, [](SynthEngine &s, float v) { s.processFilterEnvDecay(v); }},
            {FilterEnvSustain, [](SynthEngine &s, float v) { s.processFilterEnvSustain(v); }},
            {FilterEnvRelease, [](SynthEngine &s, float v) { s.processFilterEnvRelease(v); }},
            {PanVoice1, [](SynthEngine &s, float v) { s.processPan(v, 1); }},
            {PanVoice2, [](SynthEngine &s, float v) { s.processPan(v, 2); }},
            {PanVoice3, [](SynthEngine &s, float v) { s.processPan(v, 3); }},
            {PanVoice4, [](SynthEngine &s, float v) { s.processPan(v, 4); }},
            {PanVoice5, [](SynthEngine &s, float v) { s.processPan(v, 5); }},
            {PanVoice6, [](SynthEngine &s, float v) { s.processPan(v, 6); }},
            {PanVoice7, [](SynthEngine &s, float v) { s.processPan(v, 7); }},
            {PanVoice8, [](SynthEngine &s, float v) { s.processPan(v, 8); }},
        };

        return handlers;
    }

    IParameterState &parameterState;
    IProgramState &programState;
    ParameterUpdateHandler updateHandler;
    SynthEngine &engine;
};

#endif // OBXF_SRC_PARAMETER_PARAMETERCOORDINATOR_H
