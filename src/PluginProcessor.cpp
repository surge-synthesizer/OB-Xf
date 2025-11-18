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
#include "sst/plugininfra/version_information.h"

#if OBLOG_TO_FILE
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
      paramAlgos(std::make_unique<ParameterAlgos>(*paramAdapter)),
      midiHandler(synth, bindings, *paramAdapter, *utils),
      state(std::make_unique<StateManager>(this))
{
    OBLOG(general, "OB-Xf startup");
    OBLOG(general, "version=" << sst::plugininfra::VersionInformation::project_version_and_hash);

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

    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(true);
    paramAdapter->getParameterUpdateHandler().updateParameters(true);
    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(false);

    synth.setSampleRate(static_cast<float>(sampleRate));
    midiHandler.setSampleRate(sampleRate);
}

void ObxfAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    paramAdapter->getParameterUpdateHandler().updateParameters();

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

int ObxfAudioProcessor::getNumPrograms() { return utils->lastFactoryPatch + 1; }
int ObxfAudioProcessor::getCurrentProgram() { return currentDawProgram; }
void ObxfAudioProcessor::setCurrentProgram(const int index)
{
    if (index < 0 || index > utils->lastFactoryPatch + 1 ||
        index > utils->patchesAsLinearList.size() + 1)
        return;
    currentDawProgram = index;
    if (index == 0)
    {
        utils->initializePatch();
    }
    else
    {
        utils->loadPatch(utils->patchesAsLinearList[index - 1]);
    }
}
const juce::String ObxfAudioProcessor::getProgramName(const int index)
{
    if (index < 0 || index > utils->lastFactoryPatch || index > utils->patchesAsLinearList.size())
        return "ERR";
    if (index == 0)
        return INIT_PATCH_NAME;
    return utils->patchesAsLinearList[index - 1].displayName;
}

void ObxfAudioProcessor::changeProgramName(const int index, const juce::String &newName)
{
    OBLOG(rework, "changeProgramName");
}

void ObxfAudioProcessor::applyActiveProgramValuesToJUCEParameters()
{
    juce::ScopedValueSetter<bool> svs(isHostAutomatedChange, false);
    if (!paramAdapter->getParameterUpdateHandler().isFIFOClear())
    {
        OBLOG(params, "Defering applying for update");
        juce::Timer::callAfterDelay(50, [this]() { applyActiveProgramValuesToJUCEParameters(); });
        return;
    }

    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(true);
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

    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(false);
}

void ObxfAudioProcessor::processActiveProgramChanged()
{
    applyActiveProgramValuesToJUCEParameters();
    sendChangeMessageWithUndoSuppressed();
}

void ObxfAudioProcessor::sendChangeMessageWithUndoSuppressed()
{
    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        // we can trigger the listeners synchronously
        paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(true);
        sendSynchronousChangeMessage();
        paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(false);
    }
    else
    {
        // We know the message queue is ordered so this should toggle
        // around the send change message.
        juce::MessageManager::callAsync(
            [this]() { paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(true); });
        sendChangeMessage();
        juce::MessageManager::callAsync(
            [this]() { paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(false); });
    }
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

void ObxfAudioProcessor::handleMIDIProgramChange(const int programNumber)
{
    OBLOG(rework, __func__ << " is midi response handler doing nothing with " << programNumber);
}

void ObxfAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    midiHandler.snapLags();
    state->collectDAWExtraStateFromInstance();
    state->getPluginStateInformation(destData);
}

void ObxfAudioProcessor::setStateInformation(const void *data, const int sizeInBytes)
{
    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(true);
    state->setPluginStateInformation(data, sizeInBytes);
    state->applyDAWExtraStateToInstance();
    paramAdapter->getParameterUpdateHandler().setSupressGestureToUndo(false);

    auto pn = activeProgram.getName();
    if (pn.isEmpty())
    {
        pn = INIT_PATCH_NAME;
    }
    OBLOG(patches, "Traversing patch list to find '" << pn << "'");
    for (auto i = 0U; i < utils->patchesAsLinearList.size(); ++i)
    {
        if (utils->patchesAsLinearList[i].displayName == pn)
        {
            OBLOG(patches, "Found patch " << i);
            lastLoadedProgram = i;
            break;
        }
    }
}

void ObxfAudioProcessor::initializeMidiCallbacks()
{
    midiHandler.handleMIDIProgramChangeCallback = [this](const int programNumber) {
        handleMIDIProgramChange(programNumber);
    };
}

void ObxfAudioProcessor::initializeUtilsCallbacks()
{
    utils->hostUpdateCallback = [this](int idx) {
        lastLoadedProgram = idx;
        if (idx < (int)utils->lastFactoryPatch)
        {
            currentDawProgram = idx + 1;
            OBLOG(patches, "set currentDawProgram to " << currentDawProgram);
            updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
        }
    };

    utils->loadMemoryBlockCallback = [this](juce::MemoryBlock &mb) {
        return state->loadFromMemoryBlock(mb);
    };

    utils->getProgramStateInformation = [this](juce::MemoryBlock &mb) {
        state->getProgramStateInformation(mb);
    };

    utils->copyTruncatedProgramNameToFXPBuffer = [this](char *buffer, const int maxSize) {
        activeProgram.getName().copyToUTF8(buffer, maxSize);
    };

    utils->resetPatchToDefault = [this]() {
        activeProgram.setToDefaultPatch();
        // we send here since with other patch loads we send
        // inside setStateInformation
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
    paramAlgos->randomizeToAlgo(algo);
    sendChangeMessage();
}

void ObxfAudioProcessor::panSetter(PanAlgos alg)
{
    paramAlgos->voicePanSetter(alg);
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
