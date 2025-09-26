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

#ifndef OBXF_SRC_PARAMETER_PARAMETERADAPTER_H
#define OBXF_SRC_PARAMETER_PARAMETERADAPTER_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <random>
#include <set>
#include "engine/SynthEngine.h"

#include "IParameterState.h"
#include "IProgramState.h"
#include "SynthParam.h"
#include "ParameterManager.h"
#include "ValueAttachment.h"
#include "ParameterList.h"

enum RandomAlgos
{
    EVERYTHING,
    A_SMIDGE,
    A_BIT_MORE,
    PANS,
};

class ParameterManagerAdapter
{
  public:
    ValueAttachment<bool> midiLearnAttachment{};
    std::atomic<bool> midiLearnChanged{false};

    ParameterManagerAdapter(IParameterState &paramState, IProgramState &progState,
                            juce::AudioProcessor &processor, SynthEngine &synth)
        : parameterState(paramState), programState(progState),
          paramManager(processor, ParameterList), engine(synth)
    {
        setupParameterCallbacks();
    }

    void setEngineParameterValue(SynthEngine & /*synth*/, const juce::String &paramId,
                                 float newValue, bool notifyToHost = false)
    {
        if (!parameterState.getMidiControlledParamSet())
        {
            parameterState.setLastUsedParameter(paramId);
        }

        auto *param = paramManager.getParameter(paramId);

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

    void updateParameters(bool force = false) { paramManager.updateParameters(force); }

    void clearFIFO() { paramManager.clearFiFO(); }

    ParameterManager &getParameterManager() { return paramManager; }
    const ParameterManager &getParameterManager() const { return paramManager; }

    void queue(const juce::String &paramID, const float value)
    {
        getParameterManager().queueParameterChange(paramID, value);
    }

    juce::RangedAudioParameter *getParameter(const juce::String &paramID) const
    {
        return paramManager.getParameter(paramID);
    }

    void randomizeToAlgo(RandomAlgos algo)
    {
        switch (algo)
        {
        case PANS:
            randomizePans();
            break;
        case EVERYTHING:
        {
            std::uniform_real_distribution dist(0.f, 1.f);
            for (const auto &paramInfo : ParameterList)
            {
                auto par = paramManager.getParameter(paramInfo.ID);
                par->beginChangeGesture();
                par->setValueNotifyingHost(par->convertFrom0to1(dist(rng)));
                par->endChangeGesture();
            }
        }
        break;
        case A_SMIDGE:
        case A_BIT_MORE:
            // These two just modify a patch subset and only by a bit
            {
                static const std::set<juce::String> excludedFloatIDs{
                    ID::Volume,
                    ID::Transpose,
                    ID::Tune,
                };

                static const std::set<juce::String> excludedIntIDs{
                    ID::HQMode,    ID::BendUpRange,  ID::BendDownRange, ID::LFO1TempoSync,
                    ID::Polyphony, ID::UnisonVoices, ID::Unison};

                float chg = 0.05;
                float prob = 0.2;
                float intProb = 0.1;
                if (algo == A_BIT_MORE)
                {
                    chg = 0.1;
                    prob = 0.4;
                    intProb = 0.2;
                }
                std::uniform_real_distribution dist(-1.f, 1.f);
                std::uniform_real_distribution diceDist(0.f, 1.f);

                for (const auto &paramInfo : ParameterList)
                {
                    if (paramInfo.meta.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
                    {
                        if (excludedFloatIDs.find(paramInfo.ID) != excludedFloatIDs.end())
                            continue;

                        auto diceRoll = diceDist(rng);
                        if (diceRoll < prob)
                        {
                            auto par = paramManager.getParameter(paramInfo.ID);
                            auto oval = par->getValue();
                            auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);
                            par->beginChangeGesture();
                            par->setValueNotifyingHost(val);
                            par->endChangeGesture();
                        }
                    }
                    else
                    {
                        if (excludedIntIDs.find(paramInfo.ID) != excludedIntIDs.end())
                            continue;

                        // purposefully set the bar higher on ints by doing two dice rolls
                        auto diceRoll = diceDist(rng);
                        if (diceRoll > intProb)
                        {
                            continue;
                        }

                        if (paramInfo.ID.toStdString() == ID::Polyphony ||
                            paramInfo.ID.toStdString() == ID::UnisonVoices ||
                            paramInfo.ID.toStdString() == ID::Unison)
                        {
                            // do these ones as a group. They are excluded currenclty
                            // but keep this code in case we want a level where it isnt
                            auto parPol = paramManager.getParameter(ID::Polyphony);
                            auto parUni = paramManager.getParameter(ID::UnisonVoices);
                            auto parUnB = paramManager.getParameter(ID::Unison);
                            auto nv = std::clamp(dist(rng), 0.f, 1.f);
                            auto nu = std::clamp(
                                dist(rng) * std::min(1.f, nv * MAX_VOICES / MAX_UNISON), 0.f, 1.f);

                            parPol->beginChangeGesture();
                            parPol->setValueNotifyingHost(nv);
                            parPol->endChangeGesture();
                            parUni->beginChangeGesture();
                            parUni->setValueNotifyingHost(nu);
                            parUni->endChangeGesture();
                            parUnB->beginChangeGesture();
                            parUnB->setValueNotifyingHost(dist(rng) > 0.5 ? 1.f : 0.f);
                            parUnB->endChangeGesture();
                        }
                        else
                        {
                            auto par = paramManager.getParameter(paramInfo.ID);
                            par->beginChangeGesture();
                            par->setValueNotifyingHost(dist(rng));
                            par->endChangeGesture();
                        }
                    }
                }
            }
            break;
        }
    }

    void randomizePans()
    {
        std::uniform_real_distribution dist(-1.0f, 1.0f);
        for (auto *param : getPanParams())
        {
            float res = dist(rng);
            res = res * res * res;
            res = (res + 1.0f) / 2.0f;
            param->beginChangeGesture();
            param->setValueNotifyingHost(res);
            param->endChangeGesture();
        }
    }

    void resetPansToDefault()
    {
        for (auto *param : getPanParams())
        {
            param->setValueNotifyingHost(0.5f);
        }
    }

  private:
    void setupParameterCallbacks()
    {
        for (const auto &paramInfo : ParameterList)
        {
            const juce::String &paramId = paramInfo.ID;
            paramManager.addParameterCallback(
                paramId, "PROGRAM", [this, paramId](const float newValue, bool /*forced*/) {
                    processParameterChange(engine, paramId, newValue);
                    this->programState.updateProgramValue(paramId, newValue);
                });
        }
    }

    void processParameterChange(SynthEngine &synth, const juce::String &paramId,
                                const float newValue)
    {
        using Handler = std::function<void(SynthEngine &, float)>;
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
            {RingModMix, [](SynthEngine &s, float v) { s.processRingModMix(v); }},
            {NoiseMix, [](SynthEngine &s, float v) { s.processNoiseMix(v); }},
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
            {FilterKeyFollow, [](SynthEngine &s, float v) { s.processFilterKeyFollow(v); }},
            {Osc1Mix, [](SynthEngine &s, float v) { s.processOsc1Mix(v); }},
            {Osc2Mix, [](SynthEngine &s, float v) { s.processOsc2Mix(v); }},
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

        const std::string idStr = paramId.toStdString();
        if (const auto it = handlers.find(idStr); it != handlers.end())
        {
            it->second(synth, newValue);
        }
    }

    std::vector<juce::RangedAudioParameter *> getPanParams() const
    {
        std::vector<juce::RangedAudioParameter *> panParams;
        for (const auto &paramInfo : ParameterList)
        {
            if (paramInfo.meta.hasFeature(IS_PAN))
            {
                if (auto *param = paramManager.getParameter(paramInfo.ID))
                    panParams.push_back(param);
            }
        }
        return panParams;
    }

    std::mt19937 rng{std::random_device{}()};

    IParameterState &parameterState;
    IProgramState &programState;
    ParameterManager paramManager;
    SynthEngine &engine;
};

#endif // OBXF_SRC_PARAMETER_PARAMETERAdapter_H
