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

#include "PluginProcessor.h"
#include "Constants.h"
#include "PluginEditor.h"

#if BACONPAUL_IS_DEBUGGING_IN_LOGIC
std::ofstream logHack;
#endif

ObxfAudioProcessor::ObxfAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      utils(std::make_unique<Utils>()),
      paramAdapter(std::make_unique<ParameterManagerAdapter>(*this, *this, *this, synth)),
      midiHandler(synth, bindings, *paramAdapter, *utils),
      state(std::make_unique<StateManager>(this))
{
#if BACONPAUL_IS_DEBUGGING_IN_LOGIC
    if (!logHack.is_open())
        logHack.open("/tmp/log.txt");
    LOGH("ObxfAudioProcessor::ObxfAudioProcessor");
#endif

    isHostAutomatedChange = true;

    initializeCallbacks();

    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;

    midiHandler.initMidi();
}
#endif

ObxfAudioProcessor::~ObxfAudioProcessor() = default;

void ObxfAudioProcessor::prepareToPlay(const double sampleRate, const int /*samplesPerBlock*/)
{
    midiHandler.prepareToPlay();

    paramAdapter->getParameterManager().setSupressGestureToUndo(true);
    paramAdapter->updateParameters(true);
    paramAdapter->getParameterManager().setSupressGestureToUndo(false);

    synth.setSampleRate(static_cast<float>(sampleRate));
    midiHandler.setSampleRate(sampleRate);
}

void ObxfAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    paramAdapter->updateParameters();

    int samplePos = 0;
    const int numSamples = buffer.getNumSamples();
    float *channelData1 = buffer.getWritePointer(0);
    float *channelData2 = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : channelData1;

    bool gotTransportInfo{false};
    if (const auto *playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (position->getBpm() && position->getPpqPosition())
            {
                gotTransportInfo = true;
                auto p = *(position->getPpqPosition());
                auto b = *(position->getBpm());
                bool resetPosition = false;

                if ((!wasPlayingLastFrame && position->getIsPlaying()) || (p < lastPPQPosition))
                {
                    resetPosition = true;
                }

                wasPlayingLastFrame = position->getIsPlaying();
                lastPPQPosition = p;
                synth.setPlayHead(b, p, resetPosition);
            }
        }
    }

    if (!gotTransportInfo)
    {
        bool resetPosition = false;
        if (!wasPlayingLastFrame || syntheticPPQPosition < 0)
        {
            wasPlayingLastFrame = true;
            resetPosition = true;
        }
        auto b = 120.0;
        if (syntheticPPQPosition < 0)
        {
            syntheticPPQPosition = 0;
        }
        else
        {
            syntheticPPQPosition += buffer.getNumSamples() * b / (60 * getSampleRate());
        }
        synth.setPlayHead(b, syntheticPPQPosition, resetPosition);
    }

    auto it = midiMessages.begin();
    bool hasMidiMessage = (it != midiMessages.end());

    synth.getMotherboard()->tuning.updateMTSESPStatus();

    while (samplePos < numSamples)
    {
        if (hasMidiMessage)
        {
            midiHandler.processMidiPerSample(&it, midiMessages, samplePos);
            hasMidiMessage = (it != midiMessages.end());
        }
        midiHandler.processLags();

        synth.processSample(channelData1 + samplePos, channelData2 + samplePos);
        ++samplePos;
    }

    assert(!hasMidiMessage);

    if (uiState.editorAttached)
    {
        uiState.lastUpdate += numSamples;
        if (uiState.lastUpdate >= uiState.updateInterval)
        {
            uiState.lastUpdate = 0;
            updateUIState();
        }
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ObxfAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================
void ObxfAudioProcessor::releaseResources() {}

const juce::String ObxfAudioProcessor::getName() const { return JucePlugin_Name; }

bool ObxfAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ObxfAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ObxfAudioProcessor::isMidiEffect() const { return false; }
double ObxfAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ObxfAudioProcessor::getNumPrograms() { return MAX_PROGRAMS; }
int ObxfAudioProcessor::getCurrentProgram()
{
    OBLOGONCE(rework, "Support current patch set in getCurrentProgram");
    return 0;
}

void ObxfAudioProcessor::loadCurrentProgramParameters()
{
    if (!paramAdapter->isFIFOClear())
    {
        juce::Timer::callAfterDelay(50, [this]() { loadCurrentProgramParameters(); });
        return;
    }

    paramAdapter->getParameterManager().setSupressGestureToUndo(true);
    const Program &prog = activeProgram;
    for (auto *param : ObxfParams(*this))
    {
        if (param)
        {
            const auto &paramId = param->paramID;
            auto it = prog.values.find(paramId);
            const float value =
                (it != prog.values.end()) ? it->second.load() : param->meta.defaultVal;

            auto v = param->convertTo0to1(param->get());
            if (v != value)
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(value);
                param->endChangeGesture();
            }
        }
    }

    paramAdapter->getParameterManager().setSupressGestureToUndo(false);
}

void ObxfAudioProcessor::processActiveProgramChanged()
{
    isHostAutomatedChange = false;
    loadCurrentProgramParameters();
    isHostAutomatedChange = true;

    sendChangeMessageWithUndoSuppressed();
}

void ObxfAudioProcessor::sendChangeMessageWithUndoSuppressed()
{
    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        // we can trigger the listeners synchronously
        paramAdapter->getParameterManager().setSupressGestureToUndo(true);
        sendSynchronousChangeMessage();
        paramAdapter->getParameterManager().setSupressGestureToUndo(false);
    }
    else
    {
        // We know the message queue is ordered so this should toggle
        // around the send change message.
        juce::MessageManager::callAsync(
            [this]() { paramAdapter->getParameterManager().setSupressGestureToUndo(true); });
        sendChangeMessage();
        juce::MessageManager::callAsync(
            [this]() { paramAdapter->getParameterManager().setSupressGestureToUndo(false); });
    }
}

const juce::String ObxfAudioProcessor::getProgramName(const int index)
{
    OBLOG(rework, "getProgramName unimplemented for " << index);
    return activeProgram.getName();
}

void ObxfAudioProcessor::changeProgramName(const int index, const juce::String &newName)
{
    OBLOG(rework, "changeProgramName");
}

bool ObxfAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *ObxfAudioProcessor::createEditor()
{
    return new ObxfAudioProcessorEditor(*this);
}

void ObxfAudioProcessor::setEngineParameterValue(const juce::String &paramId, float newValue,
                                                 bool notifyToHost)
{
    paramAdapter->setEngineParameterValue(synth, paramId, newValue, notifyToHost);
}

bool ObxfAudioProcessor::loadFromMemoryBlock(juce::MemoryBlock &mb) const
{
    return state->loadFromMemoryBlock(mb);
}

void ObxfAudioProcessor::onProgramChange(const int programNumber)
{
    setCurrentProgram(programNumber);
}

void ObxfAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    midiHandler.snapLags();
    state->collectDAWExtraStateFromInstance();
    state->getPluginStateInformation(destData);
}

void ObxfAudioProcessor::setStateInformation(const void *data, const int sizeInBytes)
{
    paramAdapter->getParameterManager().setSupressGestureToUndo(true);
    state->setPluginStateInformation(data, sizeInBytes);
    state->applyDAWExtraStateToInstance();
    paramAdapter->getParameterManager().setSupressGestureToUndo(false);
}

void ObxfAudioProcessor::initializeMidiCallbacks()
{
    midiHandler.onProgramChangeCallback = [this](const int programNumber) {
        onProgramChange(programNumber);
    };
}

void ObxfAudioProcessor::initializeUtilsCallbacks()
{
    utils->hostUpdateCallback = [this]() {
        updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
    };

    utils->loadMemoryBlockCallback = [this](juce::MemoryBlock &mb) {
        return loadFromMemoryBlock(mb);
    };

    utils->getProgramStateInformation = [this](juce::MemoryBlock &mb) {
        state->getProgramStateInformation(mb);
    };

    utils->copyTruncatedProgramNameToFXPBuffer = [this](char *buffer, const int maxSize) {
        activeProgram.getName().copyToUTF8(buffer, maxSize);
    };

    utils->resetPatchToDefault = [this]() { activeProgram.setToDefaultPatch(); };

    utils->sendChangeMessage = [this]() {
        OBLOG(rework, "Call to sendChangeMessages - why?");
        sendChangeMessage();
    };
}

void ObxfAudioProcessor::initializeCallbacks()
{
    initializeMidiCallbacks();
    initializeUtilsCallbacks();
}

void ObxfAudioProcessor::randomizeToAlgo(RandomAlgos algo)
{
    paramAdapter->randomizeToAlgo(algo);
    sendChangeMessage();
}

void ObxfAudioProcessor::panSetter(PanAlgos alg)
{
    paramAdapter->voicePanSetter(alg);
    sendChangeMessage();
}

void ObxfAudioProcessor::updateUIState()
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        uiState.voiceStatusValue[i] = synth.getVoiceAmpEnvStatus(i);
    }
}

//==============================================================================

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new ObxfAudioProcessor(); }
