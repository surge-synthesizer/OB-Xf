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

struct ParameterAlgos
{
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
