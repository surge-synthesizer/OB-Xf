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

#ifndef OBXF_SRC_PARAMETER_PARAMETERALGOS_H
#define OBXF_SRC_PARAMETER_PARAMETERALGOS_H

#include "ParameterCoordinator.h"

#include <unordered_map>

enum RandomAlgos
{
    EVERYTHING,
    A_SMIDGE,
    A_BIT_MORE,
};

enum PanAlgos
{
    RESET_ALL = 0,
    RANDOMIZE,
    ALTERNATE_25,
    ALTERNATE_50,
    ALTERNATE_100,
    SPREAD_25,
    SPREAD_50,
    SPREAD_100,
};

enum PatchMutatorSections
{
    MUTATE_OSC = 0,
    MUTATE_MIXER,
    MUTATE_FILTER,
    MUTATE_LFOS,
    MUTATE_ENVELOPES,
    MUTATE_VOICE_VARIATION,
    MUTATE_ALL,
};

struct ParameterAlgos
{
    struct MutationProfile
    {
        float floatDelta{0.12f};
        float floatProbability{0.55f};
        float intProbability{0.12f};
    };

    ParameterCoordinator &manager;
    ParameterAlgos(ParameterCoordinator &manager) : manager(manager) {}

    void randomizeToAlgo(RandomAlgos algo)
    {
        switch (algo)
        {
        case EVERYTHING:
        {
            std::uniform_real_distribution dist(0.f, 1.f);
            for (const auto &paramInfo : ParameterList)
            {
                auto par = uh().getParameter(paramInfo.ID);
                setWithBeginEnd(par, par->convertFrom0to1(dist(rng)));
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
                    ID::HQMode,        ID::BendUpRange, ID::BendDownRange, ID::LFO1TempoSync,
                    ID::LFO2TempoSync, ID::Polyphony,   ID::UnisonVoices,  ID::Unison};

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
                std::uniform_real_distribution distU01(0.f, 1.f);

                for (const auto &paramInfo : ParameterList)
                {
                    if (paramInfo.meta.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
                    {
                        if (excludedFloatIDs.find(paramInfo.ID) != excludedFloatIDs.end())
                            continue;

                        auto diceRoll = distU01(rng);
                        if (diceRoll < prob)
                        {
                            auto par = uh().getParameter(paramInfo.ID);
                            auto oval = par->getValue();
                            auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);
                            setWithBeginEnd(par, val);
                        }
                    }
                    else
                    {
                        if (excludedIntIDs.find(paramInfo.ID) != excludedIntIDs.end())
                            continue;

                        // purposefully set the bar higher on ints by doing two dice rolls
                        auto diceRoll = distU01(rng);
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
                            auto parPol = uh().getParameter(ID::Polyphony);
                            auto parUni = uh().getParameter(ID::UnisonVoices);
                            auto parUnB = uh().getParameter(ID::Unison);
                            auto nv = std::clamp(distU01(rng), 0.f, 1.f);
                            auto nu = std::clamp(
                                dist(rng) * std::min(1.f, nv * MAX_VOICES / MAX_UNISON), 0.f, 1.f);

                            setWithBeginEnd(parPol, nv);
                            setWithBeginEnd(parUni, nu);
                            setWithBeginEnd(parUnB, dist(rng) > 0.5 ? 1.f : 0.f);
                        }
                        else
                        {
                            auto par = uh().getParameter(paramInfo.ID);

                            setWithBeginEnd(par, std::clamp(distU01(rng), 0.f, 1.f));
                        }
                    }
                }
            }
            break;
        }
    }

    void voicePanSetter(PanAlgos alg)
    {
        switch (static_cast<int>(alg))
        {
        case RESET_ALL:
        {
            for (auto *param : getPanParams())
            {
                float res = 0.f;
                res = (res + 1.0f) * 0.5f;
                setWithBeginEnd(param, res);
            }

            break;
        }
        case RANDOMIZE:
        {
            std::uniform_real_distribution dist(-1.0f, 1.0f);

            for (auto *param : getPanParams())
            {
                float res = dist(rng);
                res = res * res * res;
                res = (res + 1.0f) * 0.5f;
                setWithBeginEnd(param, res);
            }

            break;
        }
        case ALTERNATE_25:
        case ALTERNATE_50:
        case ALTERNATE_100:
        {
            int i = 0;
            float spread = (alg == ALTERNATE_25) ? 0.25f : (alg == ALTERNATE_50) ? 0.5f : 1.f;

            for (auto *param : getPanParams())
            {
                setWithBeginEnd(param, 0.5f - (spread * 0.5f) + (spread * i));

                i = 1 - i;
            }

            break;
        }
        case SPREAD_25:
        case SPREAD_50:
        case SPREAD_100:
        {
            int i = 0;
            float spread = (alg == SPREAD_25) ? 0.25f : (alg == SPREAD_50) ? 0.5f : 1.f;

            for (auto *param : getPanParams())
            {
                setWithBeginEnd(param,
                                0.5f - (spread * 0.5f) + ((spread / (MAX_PANNINGS - 1)) * i));
                i++;
            }

            break;
        }
        default:
            break;
        }
    }

    std::vector<juce::RangedAudioParameter *> getPanParams() const
    {
        std::vector<juce::RangedAudioParameter *> panParams;

        for (const auto &paramInfo : ParameterList)
        {
            if (paramInfo.meta.hasFeature(IS_PAN))
            {
                if (auto *param = uh().getParameter(paramInfo.ID))
                {
                    panParams.push_back(param);
                }
            }
        }

        return panParams;
    }

    // Patch Mutator Methods
    std::set<juce::String> getParameterIDsForSection(PatchMutatorSections section) const
    {
        std::set<juce::String> result;

        if (section == MUTATE_OSC)
        {
            result = {
                ID::Osc1Pitch,
                ID::Osc2Detune,
                ID::Osc2Pitch,
                ID::Osc1SawWave,
                ID::Osc2SawWave,
                ID::Osc1PulseWave,
                ID::Osc2PulseWave,
                ID::OscPW,
                ID::Osc2PWOffset,
                ID::EnvToPitchAmount,
                ID::EnvToPitchBothOscs,
                ID::EnvToPitchInvert,
                ID::EnvToPWAmount,
                ID::EnvToPWBothOscs,
                ID::EnvToPWInvert,
                ID::OscCrossmod,
                ID::OscSync,
                ID::OscBrightness,
            };
        }
        else if (section == MUTATE_MIXER)
        {
            result = {ID::Osc1Vol, ID::Osc2Vol, ID::RingModVol, ID::NoiseVol, ID::NoiseColor};
        }
        else if (section == MUTATE_FILTER)
        {
            result = {
                ID::Filter4PoleMode,    ID::FilterCutoff,    ID::FilterResonance,
                ID::FilterEnvAmount,    ID::FilterKeyTrack,  ID::FilterMode,
                ID::Filter2PoleBPBlend, ID::Filter2PolePush, ID::Filter4PoleXpander,
                ID::FilterXpanderMode,  ID::FilterEnvInvert,
            };
        }
        else if (section == MUTATE_LFOS)
        {
            result = {
                ID::VibratoWave,
                ID::VibratoRate,
                ID::LFO1TempoSync,
                ID::LFO1Rate,
                ID::LFO1ModAmount1,
                ID::LFO1ModAmount2,
                ID::LFO1Wave1,
                ID::LFO1Wave2,
                ID::LFO1Wave3,
                ID::LFO1PW,
                ID::LFO1ToOsc1Pitch,
                ID::LFO1ToOsc2Pitch,
                ID::LFO1ToFilterCutoff,
                ID::LFO1ToOsc1PW,
                ID::LFO1ToOsc2PW,
                ID::LFO1ToVolume,
                ID::LFO2TempoSync,
                ID::LFO2Wave1,
                ID::LFO2Wave2,
                ID::LFO2Wave3,
                ID::LFO2PW,
                ID::LFO2Rate,
                ID::LFO2ModAmount1,
                ID::LFO2ModAmount2,
                ID::LFO2ToOsc1Pitch,
                ID::LFO2ToOsc2Pitch,
                ID::LFO2ToFilterCutoff,
                ID::LFO2ToOsc1PW,
                ID::LFO2ToOsc2PW,
                ID::LFO2ToVolume,
            };
        }
        else if (section == MUTATE_ENVELOPES)
        {
            result = {
                ID::FilterEnvInvert,  ID::FilterEnvAttack,  ID::FilterEnvDecay,
                ID::FilterEnvSustain, ID::FilterEnvRelease, ID::FilterEnvAttackCurve,
                ID::VelToFilterEnv,   ID::AmpEnvAttack,     ID::AmpEnvDecay,
                ID::AmpEnvSustain,    ID::AmpEnvRelease,    ID::AmpEnvAttackCurve,
                ID::VelToAmpEnv,
            };
        }
        else if (section == MUTATE_VOICE_VARIATION)
        {
            result = {
                ID::UnisonDetune, ID::PortamentoSlop, ID::FilterSlop, ID::EnvelopeSlop,
                ID::LevelSlop,    ID::PanVoice1,      ID::PanVoice2,  ID::PanVoice3,
                ID::PanVoice4,    ID::PanVoice5,      ID::PanVoice6,  ID::PanVoice7,
                ID::PanVoice8,
            };
        }

        return result;
    }

    void mutatePatchSection(PatchMutatorSections section)
    {
        auto paramIds = getParameterIDsForSection(section);
        if (paramIds.empty())
            return;

        auto profile = getMutationProfile(section);
        std::uniform_real_distribution<float> distSigned(-1.f, 1.f);
        std::uniform_real_distribution<float> distU01(0.f, 1.f);

        for (const auto &paramInfo : ParameterList)
        {
            if (paramIds.find(paramInfo.ID) == paramIds.end())
                continue;

            auto *par = uh().getParameter(paramInfo.ID);
            if (!par)
                continue;

            const auto currentValue = par->getValue();

            if (paramInfo.meta.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
            {
                if (distU01(rng) > profile.floatProbability)
                    continue;

                auto nextValue =
                    std::clamp(currentValue + distSigned(rng) * profile.floatDelta, 0.f, 1.f);
                nextValue = applyMutationGuardRails(paramInfo.ID, nextValue);
                setWithBeginEnd(par, nextValue);
            }
            else
            {
                if (distU01(rng) > profile.intProbability)
                    continue;

                if (paramInfo.meta.type == sst::basic_blocks::params::ParamMetaData::BOOL)
                {
                    setWithBeginEnd(par, currentValue > 0.5f ? 0.f : 1.f);
                }
                else
                {
                    auto nextValue = std::clamp(currentValue + distSigned(rng) * 0.25f, 0.f, 1.f);
                    nextValue = applyMutationGuardRails(paramInfo.ID, nextValue);
                    setWithBeginEnd(par, nextValue);
                }
            }
        }
    }

    MutationProfile getMutationProfile(PatchMutatorSections section) const
    {
        switch (section)
        {
        case MUTATE_OSC:
            return MutationProfile{0.10f, 0.50f, 0.08f};
        case MUTATE_MIXER:
            return MutationProfile{0.12f, 0.70f, 0.05f};
        case MUTATE_FILTER:
            return MutationProfile{0.10f, 0.60f, 0.10f};
        case MUTATE_LFOS:
            return MutationProfile{0.16f, 0.55f, 0.14f};
        case MUTATE_ENVELOPES:
            return MutationProfile{0.10f, 0.60f, 0.10f};
        case MUTATE_VOICE_VARIATION:
            return MutationProfile{0.18f, 0.55f, 0.10f};
        case MUTATE_ALL:
        default:
            return MutationProfile{};
        }
    }

    float applyMutationGuardRails(const juce::String &paramId, float value) const
    {
        static const std::unordered_map<std::string, std::pair<float, float>> ranges{
            {ID::Osc1Vol, {0.15f, 1.f}},       {ID::Osc2Vol, {0.15f, 1.f}},
            {ID::FilterCutoff, {0.05f, 1.f}},  {ID::AmpEnvAttack, {0.f, 0.70f}},
            {ID::AmpEnvDecay, {0.f, 0.85f}},   {ID::AmpEnvSustain, {0.20f, 1.f}},
            {ID::AmpEnvRelease, {0.f, 0.85f}},
        };

        if (auto it = ranges.find(paramId.toStdString()); it != ranges.end())
            return std::clamp(value, it->second.first, it->second.second);

        return value;
    }

    void setWithBeginEnd(auto *param, float value)
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(value);
        param->endChangeGesture();
    }

    ParameterUpdateHandler &uh() { return manager.getParameterUpdateHandler(); }
    const ParameterUpdateHandler &uh() const { return manager.getParameterUpdateHandler(); }

    std::mt19937 rng{std::random_device{}()};
};

#endif // OB_XF_PARAMETERALGOS_H
