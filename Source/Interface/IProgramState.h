#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class IProgramState {
public:
    virtual ~IProgramState() = default;

    virtual void updateProgramValue(int index, float value) = 0;

    virtual juce::AudioProcessorValueTreeState &getValueTreeState() = 0;
};
