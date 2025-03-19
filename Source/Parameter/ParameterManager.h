#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterInfo.h"
#include "FIFO.h"


class ParameterManager final : public juce::AudioProcessorValueTreeState::Listener {
public:
    using Callback = std::function<void(float value, bool forced)>;

    ParameterManager(juce::AudioProcessor &audioProcessor,
                     const juce::String &identifier,
                     const std::vector<ParameterInfo> &parameters);

    ParameterManager() = delete;

    ~ParameterManager() override;

    bool registerParameterCallback(const juce::String &ID, Callback cb);

    void flushParameterQueue();

    void clearFiFO()
    {
        fifo.clear();
    }

    void updateParameters(bool force = false);

    void clearParameterQueue();

    const std::vector<ParameterInfo> &getParameters() const;

    juce::AudioProcessorValueTreeState &getAPVTS();

    void parameterChanged(const juce::String &parameterID, float newValue) override;

private:
    juce::AudioProcessorValueTreeState apvts;
    std::vector<ParameterInfo> parameters;
    FIFO<128> fifo;
    std::unordered_map<juce::String, Callback> callbacks;

    JUCE_DECLARE_NON_COPYABLE(ParameterManager)

    JUCE_DECLARE_NON_MOVEABLE(ParameterManager)

    JUCE_LEAK_DETECTOR(ParameterManager)
};
