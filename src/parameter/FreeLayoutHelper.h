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

#ifndef OBXF_SRC_PARAMETER_FREELAYOUTHELPER_H
#define OBXF_SRC_PARAMETER_FREELAYOUTHELPER_H

#include "SynthParam.h"
#include "Utils.h"

static juce::AudioProcessorValueTreeState::ParameterLayout

createParameterLayout(const std::vector<ParameterInfo> &infos)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    size_t index{0};
    for (const auto &info : infos)
    {
        // Fix that inc and skew
        juce::NormalisableRange<float> range;

        range = juce::NormalisableRange<float>{
            info.meta.minVal, info.meta.maxVal,
            (info.meta.type == sst::basic_blocks::params::ParamMetaData::Type::FLOAT) ? 0.00001f
                                                                                      : 1.f,
            1.f};

        auto parameter = std::make_unique<ObxfParameterFloat>(juce::ParameterID{info.ID, 1}, range,
                                                              index++, info.meta);
        params.push_back(std::move(parameter));
    }

    return {params.begin(), params.end()};
}

#endif // OBXF_SRC_PARAMETER_FREELAYOUTHELPER_H
