#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "juce_dsp/juce_dsp.h"
#include "Engine/SynthEngine.h"
#include "Engine/midiMap.h"
#include "Engine/ObxdBank.h"
#include "Constants.h"
#include "ParamaterManager.h"





class ObxdAudioProcessor final : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener,
                                 virtual public juce::ChangeBroadcaster,
                                 public IParameterState,
                                 public IProgramState
{
public:
    ObxdAudioProcessor();
    ~ObxdAudioProcessor() override;

	using juce::ChangeBroadcaster::sendChangeMessage;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
	void parameterChanged(const juce::String &parameterID, float newValue) override;
    //==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void processMidiPerSample(juce::MidiBufferIterator* iter, const juce::MidiBuffer& midiBuffer, int samplePos);
    bool getNextEvent(juce::MidiBufferIterator* iter, const juce::MidiBuffer& midiBuffer, int samplePos);
    void initAllParams();
    void initMidi();

    MidiMap &getMidiMap(){ return bindings; }
	const ObxdBank& getPrograms() const { return programs; }
	MidiMap bindings;

	// IPARAMETERSTATE IPROGRAMSTATE
	bool getMidiControlledParamSet() const override{return midiControlledParamSet;}
	void setLastUsedParameter(const int param) override{lastUsedParameter = param;}
	bool getIsHostAutomatedChange() const override{return isHostAutomatedChange;}
	void updateProgramValue(const int index, const float value) override {
		programs.currentProgramPtr->values[index] = value;
	}
	juce::AudioProcessorValueTreeState& getValueTreeState() override{return apvtState;}
	// IPARAMETERSTATE IPROGRAMSTATE


  private:
	bool isHostAutomatedChange;

	int lastMovedController;
	int lastUsedParameter;

	std::unique_ptr<juce::MidiMessage> nextMidi;
	std::unique_ptr<juce::MidiMessage> midiMsg;

	bool midiControlledParamSet;

	bool hasMidiMessage{};
	int midiEventPos{};

	SynthEngine synth;
	ObxdBank programs;

	juce::String currentMidiPath;

	juce::AudioProcessorValueTreeState apvtState;
	juce::UndoManager                  undoManager;

	ParameterManager paramManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObxdAudioProcessor)
};