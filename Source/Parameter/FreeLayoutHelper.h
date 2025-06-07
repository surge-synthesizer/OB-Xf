#pragma once
#include "SynthParam.h"

inline float logsc(const float param, const float min, const float max, const float skew = 1.0f) {
    if (skew == 1.0f)
        return min + (max - min) * param;

    return min + (max - min) * std::pow(param, skew);
}


static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(
    const std::vector<ParameterInfo> &infos) {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    for (const auto &info: infos) {
        auto range = juce::NormalisableRange<float>{
            info.min, info.max, info.inc, info.skw
        };

        auto stringFromValue = [id = info.ID](const float value, int /*maxStringLength*/) {
            juce::String result;

            if (id == SynthParam::ID::VibratoRate) {
                result = juce::String{logsc(value, 3, 10), 2} + " Hz";
                return result;
            }
            if (id == SynthParam::ID::Octave) {
                result = juce::String{(static_cast<float>(juce::roundToInt(value * 4)) - 2) * 12.f, 0} + " " + SynthParam::Units::Semitones;
                return result;
            }
            if (id == SynthParam::ID::Tune) {
                const float cents = value * 200.0f - 100.0f; // Maps 0-1 to -100 to +100
                return juce::String{cents, 1} + " " + SynthParam::Units::Cents;
            }
            if (id == SynthParam::ID::NoiseMix) {
                if (const auto decibels = juce::Decibels::gainToDecibels(logsc(value, 0, 1, 35)); decibels < -80) result = juce::String("-Inf");
                else result = juce::String{decibels, 2} + " " + SynthParam::Units::Decibels;
                return result;
            }
            if (id == SynthParam::ID::Osc1Mix || id == SynthParam::ID::Osc2Mix) {
                if (const auto decibels = juce::Decibels::gainToDecibels(value); decibels < -80) result = juce::String("-Inf");
                else result = juce::String{decibels, 2} + " " + SynthParam::Units::Decibels;
                return result;
            }
            if (id == SynthParam::ID::LfoFrequency) {
                result = juce::String{logsc(value, 0, 50, 120), 2} + " " + SynthParam::Units::Hz;
                return result;
            }
            if (id == SynthParam::ID::Pan1 || id == SynthParam::ID::Pan2 ||
                id == SynthParam::ID::Pan3 || id == SynthParam::ID::Pan4 ||
                id == SynthParam::ID::Pan5 || id == SynthParam::ID::Pan6 ||
                id == SynthParam::ID::Pan7 || id == SynthParam::ID::Pan8) {
                if (const auto pan = value - 0.5f; pan < 0.f) result = juce::String{pan, 2} + " (Left)";
                else if (pan > 0.f) result = juce::String{pan, 2} + " (Right)";
                else result = juce::String{pan, 2} + " (Center)";
                return result;
            }
            if (id == SynthParam::ID::Osc1Pitch || id == SynthParam::ID::Osc2Pitch) {
                result = juce::String{((value * 4) - 2) * 12.f, 1} + " " + SynthParam::Units::Semitones;
                return result;
            }

            result = juce::String{static_cast<int>(juce::jmap(value, 0.f, 127.f))};
            return result;
        };

        auto parameter = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{info.ID, 1},
            info.name,
            range,
            info.def,
            info.unit,
            juce::AudioProcessorParameter::genericParameter,
            stringFromValue
        );

        params.push_back(std::move(parameter));
    }

    return {params.begin(), params.end()};
}
