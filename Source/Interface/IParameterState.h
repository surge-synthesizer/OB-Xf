#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class IParameterState : virtual public juce::ChangeBroadcaster {
public:
    ~IParameterState() override = default;

    [[nodiscard]] virtual bool getMidiControlledParamSet() const = 0;

    virtual void setLastUsedParameter(int param) = 0;

    [[nodiscard]] virtual bool getIsHostAutomatedChange() const = 0;
};
