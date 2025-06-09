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

#ifndef OBXF_SRC_PLUGINPROCESSOR_H
#define OBXF_SRC_PLUGINPROCESSOR_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "juce_dsp/juce_dsp.h"
#include "Engine/SynthEngine.h"
#include "Engine/midiMap.h"
#include "Engine/ObxfBank.h"
#include "Constants.h"
#include "ParameterAdaptor.h"
#include "MidiHandler.h"
#include "Utils.h"
#include "StateManager.h"

class ObxfAudioProcessor final : public juce::AudioProcessor,
                                 public juce::AudioProcessorValueTreeState::Listener,
                                 public IParameterState,
                                 public IProgramState
{
  public:
    ObxfAudioProcessor();

    ~ObxfAudioProcessor() override;

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

    const ObxfBank &getPrograms() const { return programs; }
    ObxfBank &getPrograms() { return programs; }
    MidiMap bindings;

    bool getMidiControlledParamSet() const override
    {
        return midiHandler.getMidiControlledParamSet();
    }
    void setLastUsedParameter(const int param) override { midiHandler.setLastUsedParameter(param); }
    bool getIsHostAutomatedChange() const override { return isHostAutomatedChange; }

    void updateProgramValue(const int index, const float value) override
    {
        if (ObxfParams *prog = programs.currentProgramPtr.load())
        {
            prog->values[index] = value;
        }
    }

    juce::AudioProcessorValueTreeState &getValueTreeState() override
    {
        return paramManager->getValueTreeState();
    }
    juce::String getCurrentMidiPath() const { return midiHandler.getCurrentMidiPath(); }
    void updateMidiConfig() const { midiHandler.updateMidiConfig(); }

    void setEngineParameterValue(int index, float newValue, bool notifyToHost = false);

    bool loadFromMemoryBlock(juce::MemoryBlock &mb) const;

    Utils &getUtils() const { return *utils; }

    ParameterManagerAdaptor &getParamManager() const { return *paramManager; }

    juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

  private:
    juce::MidiKeyboardState keyboardState;
    std::atomic<bool> isHostAutomatedChange{};
    SynthEngine synth;
    ObxfBank programs;

    std::unique_ptr<Utils> utils;
    std::unique_ptr<ParameterManagerAdaptor> paramManager;
    MidiHandler midiHandler;
    juce::UndoManager undoManager;

    std::unique_ptr<StateManager> state;

    void initializeCallbacks();

    void initializeMidiCallbacks();

    void initializeUtilsCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxfAudioProcessor)
};

#endif // OBXF_SRC_PLUGINPROCESSOR_H
