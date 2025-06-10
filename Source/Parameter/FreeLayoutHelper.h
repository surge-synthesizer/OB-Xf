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

    using namespace SynthParam;

    for (const auto &info : infos)
    {
        auto range = juce::NormalisableRange<float>{info.min, info.max, info.inc, info.skw};

        auto stringFromValue = [id = info.ID](const float value, int /*maxStringLength*/) {
            juce::String result;

            if (id == ID::VibratoRate)
            {
                result = juce::String{logsc(value, 3.f, 10.f), 2} + Units::Hz;
                return result;
            }
            else if (id == ID::Octave)
            {
                result =
                    juce::String{juce::roundToInt(((value * 2.f) - 1.f) * 24.f)} + Units::Semitones;
                return result;
            }
            else if (id == ID::Tune)
            {
                const float cents = juce::jmap(value, -100.0f, 100.0f);
                return juce::String{cents, 1} + Units::Cents;
            }
            else if (id == ID::EnvelopeToPitch)
            {
                return juce::String{value * 36.f, 2} + Units::Semitones;
            }
            else if (id == ID::Oscillator2Detune)
            {
                const float cents = logsc(value, 0.001f, 0.9f) * 100.f;
                return juce::String{cents, 1} + Units::Cents;
            }
            else if (id == ID::Osc1Mix || id == ID::Osc2Mix)
            {
                if (const auto decibels = juce::Decibels::gainToDecibels(value); decibels < -96.f)
                    result = juce::String("-Inf");
                else
                    result = juce::String{decibels, 2} + Units::Decibels;
                return result;
            }
            else if (id == ID::NoiseMix)
            {
                if (const auto decibels =
                        juce::Decibels::gainToDecibels(logsc(value, 0.f, 1.f, 35.f));
                    decibels < -96.f)
                    result = juce::String("-Inf");
                else
                    result = juce::String{decibels, 2} + Units::Decibels;
                return result;
            }
            else if (id == ID::Cutoff)
            {
                result = juce::String{getPitch(linsc(value, 0.f, 120.f) - 45.f), 1} + Units::Hz;
                return result;
            }
            else if (id == ID::LfoFrequency)
            {
                result = juce::String{logsc(value, 0.f, 50.f, 120.f), 2} + Units::Hz;
                return result;
            }
            else if (id == ID::Pan1 || id == ID::Pan2 || id == ID::Pan3 || id == ID::Pan4 ||
                     id == ID::Pan5 || id == ID::Pan6 || id == ID::Pan7 || id == ID::Pan8)
            {
                const auto pan = juce::roundToInt(juce::jmap(value, -100.f, 100.f));

                if (pan < 0)
                    result = juce::String{abs(pan)} + " L";
                else if (pan > 0)
                    result = juce::String{pan} + " R";
                else
                    result = "Center";
                return result;
            }
            else if (id == ID::Osc1Pitch || id == ID::Osc2Pitch)
            {
                result = juce::String{((value * 2.f) - 1.f) * 24.f, 2} + Units::Semitones;
                return result;
            }
            else
            {
                result = juce::String{juce::jmap(value, 0.f, 100.f), 1} + Units::Percent;
            }

            return result;
        };

        auto parameter = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{info.ID, 1}, info.name, range, info.def, info.unit,
            juce::AudioProcessorParameter::genericParameter, stringFromValue);

        params.push_back(std::move(parameter));
    }

    return {params.begin(), params.end()};
}

#endif // OBXF_SRC_PARAMETER_FREELAYOUTHELPER_H
