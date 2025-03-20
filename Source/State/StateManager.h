#pragma once
#include <juce_core/juce_core.h>
#include "Constants.h"
#include "SynthEngine.h"
#include "ObxdBank.h"
#include <juce_audio_processors/juce_audio_processors.h>


class ObxdAudioProcessor;

class StateManager final : public juce::ChangeBroadcaster {
public:
    explicit StateManager(ObxdAudioProcessor *processor) : audioProcessor(processor) {
    }

    ~StateManager() override;


    StateManager(const StateManager &) = delete;

    StateManager &operator=(const StateManager &) = delete;

    bool isMemoryBlockAPreset(const juce::MemoryBlock &mb);

    bool loadFromMemoryBlock(juce::MemoryBlock &mb);

    bool restoreProgramSettings(const fxProgram *prog) const;

    void getStateInformation(juce::MemoryBlock &destData) const;

    void getCurrentProgramStateInformation(juce::MemoryBlock &destData) const;

    void setStateInformation(const void *data, int sizeInBytes);

    void setCurrentProgramStateInformation(const void *data, int sizeInBytes);

private:
    ObxdAudioProcessor *audioProcessor{nullptr};
};
