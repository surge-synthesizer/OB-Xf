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
#include <juce_dsp/juce_dsp.h>
#include <random>

#include "engine/SynthEngine.h"
#include "engine/MidiMap.h"
#include "Constants.h"
#include "ParameterAdapter.h"
#include "ParameterAlgos.h"
#include "MidiHandler.h"
#include "Utils.h"
#include "StateManager.h"

#include "configuration.h"

class ObxfAudioProcessor final : public juce::AudioProcessor,
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

    // This is the DAW program API
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;
    int currentDawProgram{0}; // doesn't change for non-factory
    int lastLoadedProgram{0}; // always the index of last utils fxp loaded

    //==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    void processActiveProgramChanged();
    void handleMIDIProgramChange(int programNumber);

    MidiMap &getMidiMap() { return bindings; }

    bool getMidiLearnParameterSelected() const override
    {
        return midiHandler.getMidiControlledParamSet();
    }
    void setLastUsedParameter(const juce::String &paramId) override
    {
        midiHandler.setLastUsedParameter(paramId);
    }

    int getLastUsedParameter() const override { return midiHandler.getLastUsedParameter(); }

    bool getIsHostAutomatedChange() const override { return isHostAutomatedChange; }

    void updateProgramValue(const juce::String &paramId, float value) override
    {
        OBLOG(paramSet,
              "Call to updateProgramValue with " << OBD(paramId) << OBD(value) << " - why?");
        activeProgram.values[paramId] = value;
    }

    juce::String getCurrentMidiPath() const { return midiHandler.getCurrentMidiPath(); }
    void updateMidiConfig() const { midiHandler.updateMidiConfig(); }

    void setEngineParameterValue(const juce::String &paramId, float newValue,
                                 bool notifyToHost = false);

    Utils &getUtils() const { return *utils; }

    ParameterManagerAdapter &getParamAdapter() const { return *paramAdapter; }

    int getCurrentPatchGroup() { return 0; }

    void randomizeToAlgo(RandomAlgos algo);
    void panSetter(PanAlgos alg);

    int selectedLFOIndex = 0;

    void applyActiveProgramValuesToJUCEParameters();

    struct ObxfParams
    {
        const juce::Array<juce::AudioProcessorParameter *> &par;

        ObxfParams(const juce::Array<juce::AudioProcessorParameter *> &p) : par(p) {}
        ObxfParams(const ObxfAudioProcessor &other) : par(other.getParameters()) {}

        struct iterator
        {
            ObxfParams &op;
            int pos{0};
            bool end{false};

            iterator(ObxfParams &p, size_t pp) : op(p), pos(pp) {}
            iterator(ObxfParams &p) : op(p), end(true) {}

            bool operator==(const iterator &other) const
            {
                if (end && end == other.end)
                    return true;
                if (!end && !other.end && pos == other.pos)
                    return true;
                return false;
            }

            bool operator!=(const iterator &other) const { return !(*this == other); }

            void operator++()
            {
                pos++;
                if (pos >= op.par.size())
                    end = true;
            }

            ObxfParameterFloat *operator*()
            {
                if (end || pos >= op.par.size())
                    return nullptr;
                return static_cast<ObxfParameterFloat *>(op.par[pos]);
            }
        };

        iterator begin() { return iterator(*this, 0); }
        iterator end() { return iterator(*this); }
    };

    struct SharedUIState
    {
        // Editor write Audio read
        std::atomic<bool> editorAttached;

        // Audio write editor read
        std::array<std::atomic<float>, MAX_VOICES> voiceStatusValue;

        // AUdio only
        int32_t lastUpdate{0};
        // 48k at 60hz is 800 samples
        static constexpr int32_t updateInterval{500};
    } uiState;

    void updateUIState();

    MidiHandler &getMidiHandler() { return midiHandler; }

    std::unique_ptr<Utils> utils;

    const Program &getActiveProgram() const { return activeProgram; }
    Program &getActiveProgram() { return activeProgram; }

  private:
    void sendChangeMessageWithUndoSuppressed();

    bool isHostAutomatedChange{true};
    SynthEngine synth;
    MidiMap bindings;

    Program activeProgram;

    std::unique_ptr<ParameterManagerAdapter> paramAdapter;
    std::unique_ptr<ParameterAlgos> paramAlgos;
    MidiHandler midiHandler;
    juce::UndoManager undoManager;

    std::unique_ptr<StateManager> state;

    bool wasPlayingLastFrame{false};
    double lastPPQPosition{-1};
    double syntheticPPQPosition{-1};

    void initializeCallbacks();

    void initializeMidiCallbacks();

    void initializeUtilsCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxfAudioProcessor)
};

#endif // OBXF_SRC_PLUGINPROCESSOR_H
