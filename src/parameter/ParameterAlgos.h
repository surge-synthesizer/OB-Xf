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

static std::array<ObxfParamFeatures, NUM_SECTIONS_TO_MUTATE> mutateSectionList{
    IS_OSCS, IS_MIXER, IS_FILTER, IS_LFOS, IS_ENVS, IS_VOICE};

struct ParameterAlgos
{
    ParameterCoordinator &manager;
    Utils &utils;

    ParameterAlgos(ParameterCoordinator &_manager, Utils &_utils) : manager(_manager), utils(_utils)
    {
    }

    void mutate(Program &program, MutateMask what)
    {
        std::uniform_int_distribution<> distPL(0, utils.patchesAsLinearList.size() - 1);
        std::vector<int> indices;
        auto tempProg = Program();

        for (int i = 0; i < NUM_SECTIONS_TO_MUTATE; i++)
        {
            if (what.test(i))
            {
                // grab a patch index for each section we want to mutate
                // make it non-repeating random picks
                auto n = distPL(rng);

                while (std::find(std::begin(indices), std::end(indices), n) != indices.end())
                {
                    n = distPL(rng);
                }

                indices.push_back(n);

                // load the patch
                auto p = utils.patchesAsLinearList[n];
                utils.loadPatch(p->file, tempProg);

                // copy only parameters belonging to a particular mutate section (osc, filter, etc.)
                for (auto &p : program.values)
                {
                    for (const auto &paramInfo : ParameterList)
                    {
                        if (paramInfo.ID.compare(p.first) == 0)
                        {
                            if (paramInfo.meta.hasFeature(mutateSectionList[i]))
                            {
                                p.second.store(tempProg.getValueById(p.first));
                            }
                        }
                    }
                }
            }
        }
    }

    constexpr float stTo01(float x) { return (x + 24.0f) / 48.0f; }

    void randomize()
    {
        // float parameters that are never randomized
        static const std::set<juce::String> excludedFloatIDs{
            ID::Volume, ID::Transpose, ID::Tune, ID::VelToFilterEnv, ID::VelToAmpEnv,
        };

        // int and bool parameters that are never randomized
        static const std::set<juce::String> excludedIntIDs{
            ID::HQMode,        ID::EnvLegatoMode, ID::NotePriority, ID::BendUpRange,
            ID::BendDownRange, ID::Polyphony,     ID::UnisonVoices, ID::Unison};

        // osc 1 pitch: only octave jumps are allowed
        static const std::array<float, 5> osc1PitchChoices = {stTo01(-24), stTo01(-12), stTo01(0),
                                                              stTo01(12), stTo01(24)};

        // osc 2 pitch: only octaves, fifths, fourths, major thirds, minor thirds are allowed
        static const std::array<float, 21> osc2PitchChoices = {
            stTo01(-24), stTo01(-19), stTo01(-17), stTo01(-16), stTo01(-15), stTo01(-12),
            stTo01(-7),  stTo01(-5),  stTo01(-4),  stTo01(-3),  stTo01(0),   stTo01(3),
            stTo01(4),   stTo01(5),   stTo01(7),   stTo01(12),  stTo01(15),  stTo01(16),
            stTo01(17),  stTo01(19),  stTo01(24)};

        // normalized threshold corresponding to 10 seconds for log-scaled envelope time
        static constexpr float kEnvTimeMax10s = 0.7374f;

        // these parameters will have a much lower chance of being set above 10 seconds
        static const std::set<juce::String> envTimeParamIDs{
            ID::FilterEnvAttack, ID::FilterEnvDecay, ID::FilterEnvRelease,
            ID::AmpEnvAttack,    ID::AmpEnvDecay,    ID::AmpEnvRelease,
        };

        float chg = 0.5f;
        float prob = 0.75f;
        float intProb = 0.5f;

        std::uniform_real_distribution dist(-1.f, 1.f);
        std::uniform_real_distribution distU01(0.f, 1.f);
        std::uniform_int_distribution<size_t> osc1Dist(0, osc1PitchChoices.size() - 1);
        std::uniform_int_distribution<size_t> osc2Dist(0, osc2PitchChoices.size() - 1);

        for (const auto &paramInfo : ParameterList)
        {
            const auto idStr = paramInfo.ID.toStdString();
            auto par = uh().getParameter(paramInfo.ID);
            auto oval = par->getValue();

            // Voice Variation section has its own dedicated randomization options
            if (paramInfo.meta.hasFeature(IS_VOICE) && idStr.compare(ID::Portamento) != 0)
            {
                continue;
            }

            if (paramInfo.meta.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
            {
                if (excludedFloatIDs.find(paramInfo.ID) != excludedFloatIDs.end())
                {
                    continue;
                }

                // limit osc 1 pitch results to just octaves
                if (idStr == ID::Osc1Pitch)
                {
                    if (distU01(rng) < prob)
                    {
                        setWithBeginEnd(par, osc1PitchChoices[osc1Dist(rng)]);
                    }

                    continue;
                }

                // limit osc 2 pitch results to most common intervals (thirds, fourths, fifths,
                // octaves)
                if (idStr == ID::Osc2Pitch)
                {
                    if (distU01(rng) < prob)
                    {
                        setWithBeginEnd(par, osc2PitchChoices[osc2Dist(rng)]);
                    }

                    continue;
                }

                if (idStr == ID::Osc2Detune)
                {
                    if (distU01(rng) < prob)
                    {
                        auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);

                        // 75% chance to reject any nudge above 0.5
                        if (val > 0.5f && distU01(rng) < 0.75f)
                        {
                            val = oval;
                        }

                        setWithBeginEnd(par, val);
                    }

                    continue;
                }

                if (idStr == ID::OscCrossmod)
                {
                    // 50% lower probability of being touched at all
                    if (distU01(rng) < prob * 0.5f)
                    {
                        const auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);

                        setWithBeginEnd(par, val);
                    }

                    continue;
                }

                // noise and ring mod volumes change half as much vs other volumes
                if (idStr == ID::NoiseVol || idStr == ID::RingModVol)
                {
                    if (distU01(rng) < prob)
                    {
                        const auto val = std::clamp(oval + dist(rng) * (chg * 0.5f), 0.f, 1.f);

                        setWithBeginEnd(par, val);
                    }

                    continue;
                }

                // ensure OscPW + Osc2PWOffset is not going above 1.f
                if (idStr == ID::Osc2PWOffset)
                {
                    if (distU01(rng) < prob)
                    {
                        const auto oscPW = uh().getParameter(ID::OscPW)->getValue();
                        const auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f - oscPW);

                        setWithBeginEnd(par, val);
                    }

                    continue;
                }

                // when Sync is active, osc 2 must stay audible
                if (idStr == ID::Osc2Vol)
                {
                    if (distU01(rng) < prob)
                    {
                        auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);

                        if (uh().getParameter(ID::OscSync)->getValue() >= 0.5f)
                        {
                            val = std::max(val, 0.25f);
                        }

                        setWithBeginEnd(par, val);
                    }
                    continue;
                }

                // allow envelope times above 10 seconds only 10% of the time
                if (envTimeParamIDs.count(paramInfo.ID))
                {
                    if (distU01(rng) < prob)
                    {
                        auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);

                        if (val > kEnvTimeMax10s && distU01(rng) < 0.9f)
                        {
                            val = kEnvTimeMax10s;
                        }

                        setWithBeginEnd(par, val);
                    }

                    continue;
                }

                // default dice roll for all other float params
                auto diceRoll = distU01(rng);

                if (diceRoll < prob)
                {
                    const auto val = std::clamp(oval + dist(rng) * chg, 0.f, 1.f);

                    setWithBeginEnd(par, val);
                }
            }
            else
            {
                if (excludedIntIDs.find(paramInfo.ID) != excludedIntIDs.end())
                {
                    continue;
                }

                if (paramInfo.ID.toStdString() == ID::Polyphony ||
                    paramInfo.ID.toStdString() == ID::UnisonVoices ||
                    paramInfo.ID.toStdString() == ID::Unison)
                {
                    // do these ones as a group. They are excluded currently
                    // but keep this code in case we want a level where it isnt
                    auto parPol = uh().getParameter(ID::Polyphony);
                    auto parUni = uh().getParameter(ID::UnisonVoices);
                    auto parUnB = uh().getParameter(ID::Unison);
                    auto nv = std::clamp(distU01(rng), 0.f, 1.f);
                    auto nu = std::clamp(dist(rng) * std::min(1.f, nv * MAX_VOICES / MAX_UNISON),
                                         0.f, 1.f);

                    setWithBeginEnd(parPol, nv);
                    setWithBeginEnd(parUni, nu);
                    setWithBeginEnd(parUnB, dist(rng) > 0.5 ? 1.f : 0.f);

                    continue;
                }

                if (idStr == ID::FilterEnvInvert)
                {
                    // 75% lower probability of being touched at all
                    if (distU01(rng) < prob * 0.25f)
                    {
                        setWithBeginEnd(par, distU01(rng));
                    }

                    continue;
                }

                // default dice roll for all other int params
                auto diceRoll = distU01(rng);

                if (diceRoll < intProb)
                {
                    auto par = uh().getParameter(paramInfo.ID);

                    setWithBeginEnd(par, distU01(rng));
                }
            }
        }

        // potentially modify filter envelope direction at the end,
        // based on filter type and cutoff values set in the loop above
        applyFilterEnvDirectionHeuristic();
    }

    // When the filter cutoff is at an extreme position the envelope should push
    // it back toward the audible range, not further away:
    //
    //   LP + low cutoff:  positive envelope
    //   HP + high cutoff: inverted envelope
    //   BP/N at extremes: invert if too high, positive if too low
    void applyFilterEnvDirectionHeuristic()
    {
        enum FilterType
        {
            LP,
            HP,
            BP // covers bandpass, notch, and asymmetric variants
        };

        const bool fourPole = uh().getParameter(ID::Filter4PoleMode)->getValue() >= 0.5f;
        const bool xpander = uh().getParameter(ID::Filter4PoleXpander)->getValue() >= 0.5f;
        const float blend = uh().getParameter(ID::FilterMode)->getValue();
        const bool twoPoleBP = uh().getParameter(ID::Filter2PoleBPBlend)->getValue() >= 0.5f;
        const float cutoff = uh().getParameter(ID::FilterCutoff)->getValue();
        const bool invertEnv = uh().getParameter(ID::FilterEnvInvert)->getValue() >= 0.5f;

        FilterType filterChar = LP;

        if (!fourPole)
        {
            if (twoPoleBP && blend >= 0.3f && blend <= 0.6f)
                filterChar = BP;
            else if (blend < 0.3f)
                filterChar = LP;
            else if (blend > 0.6f)
                filterChar = HP;
            else
                filterChar = BP; // notch / blend region
        }
        else if (!xpander)
        {
            filterChar = LP;
        }
        else
        {
            // Xpander filter modes
            // LP:      0 = LP4, 1 = LP3, 2 = LP2, 3 = LP1, 13 = N2+LP1, 14 = PH3+LP1
            // HP:      4 = HP3, 5 = HP2, 6 = HP1
            // BP/N/PH: 7 = BP4, 8 = BP2, 9 = N2, 10 = PH3, 11 = HP2+LP1, 12 = HP3+LP1
            const int xMode = static_cast<int>(std::round(
                uh().getParameter(ID::FilterXpanderMode)->getValue() * NUM_XPANDER_MODES));

            if (xMode <= 3 || xMode == 13 || xMode == 14)
            {
                filterChar = LP;
            }
            else if (xMode >= 4 && xMode <= 6)
            {
                filterChar = HP;
            }
            else
            {
                filterChar = BP;
            }
        }

        bool targetInvert{invertEnv};

        OBLOG(general, "type is " << filterChar << ", cutoff is " << cutoff << ", invert is "
                                  << targetInvert);

        switch (filterChar)
        {
        case LP:
            targetInvert = (cutoff > 0.3f) ? invertEnv : false;
            break;
        case HP:
            targetInvert = (cutoff > 0.75f) ? true : invertEnv;
            break;
        case BP:
            targetInvert = (cutoff > 0.75f) ? true : ((cutoff < 0.25) ? false : invertEnv);
            break;
        }

        OBLOG(general, "final invert is " << targetInvert);

        if (targetInvert)
        {
            setWithBeginEnd(uh().getParameter(ID::FilterEnvInvert), targetInvert);
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
