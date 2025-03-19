#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "juce_dsp/juce_dsp.h"
#include "Engine/SynthEngine.h"
#include "Engine/midiMap.h"
#include "Engine/ObxdBank.h"
#include "Constants.h"
#include "ParameterAdaptor.h"
#include "MidiHandler.h"
#include "Utils.h"
#include "StateManager.h"


class ObxdAudioProcessor final : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener,
                                 public IParameterState,
                                 public IProgramState{
public:
    ObxdAudioProcessor();

    ~ObxdAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    void setCurrentProgram(int index, bool updateHost);

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    void parameterChanged(const juce::String &parameterID, float newValue) override;

    void onProgramChange(int programNumber);


    //==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    void initAllParams();

    MidiMap &getMidiMap() { return bindings; }

    const ObxdBank &getPrograms() const { return programs; }
    ObxdBank &getPrograms() { return programs; }
    MidiMap bindings;

    bool getMidiControlledParamSet() const override { return midiHandler.getMidiControlledParamSet(); }
    void setLastUsedParameter(const int param) override { midiHandler.setLastUsedParameter(param); }
    bool getIsHostAutomatedChange() const override { return isHostAutomatedChange; }

    void updateProgramValue(const int index, const float value) override {
        programs.currentProgramPtr->values[index] = value;
    }

    juce::AudioProcessorValueTreeState &getValueTreeState() override { return paramManager->getValueTreeState(); }
    juce::String getCurrentMidiPath() const { return midiHandler.getCurrentMidiPath(); }
    void updateMidiConfig() const { midiHandler.updateMidiConfig(); }

    void setEngineParameterValue(int index, float newValue, bool notifyToHost = false);

    bool loadFromMemoryBlock(juce::MemoryBlock &mb) const;

    Utils &getUtils() const {
        return *utils;
    }



private:
    bool isHostAutomatedChange;

    SynthEngine synth;
    ObxdBank programs;

    std::unique_ptr<Utils> utils;
    std::unique_ptr<ParameterManagerAdaptor> paramManager;
    MidiHandler midiHandler;
    juce::UndoManager undoManager;

    std::unique_ptr<StateManager> state;

    void initializeCallbacks();

    void initializeMidiCallbacks();

    void initializeUtilsCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxdAudioProcessor)
};
