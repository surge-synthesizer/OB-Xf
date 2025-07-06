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
      paramAdapter(std::make_unique<ParameterManagerAdapter>(*this, *this, *this)),
      midiHandler(synth, bindings, *paramAdapter, *utils),
      state(std::make_unique<StateManager>(this)), panRng(std::random_device{}())
{
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

    paramAdapter->updateParameters(true);

    paramAdapter->setEngine(synth);

    synth.setSampleRate(static_cast<float>(sampleRate));
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

    if (const auto *playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (position->getBpm() && position->getPpqPosition())
            {
                synth.setPlayHead(*(position->getBpm()), *(position->getPpqPosition()));
            }
        }
    }

    auto it = midiMessages.begin();
    const bool hasMidiMessage = (it != midiMessages.end());

    while (samplePos < numSamples)
    {
        if (hasMidiMessage)
        {
            midiHandler.processMidiPerSample(&it, midiMessages, samplePos);
        }
        synth.processSample(channelData1 + samplePos, channelData2 + samplePos);
        ++samplePos;
    }

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
int ObxfAudioProcessor::getCurrentProgram() { return programs.currentProgram; }

void ObxfAudioProcessor::setCurrentProgram(const int index)
{
    programs.currentProgram = index;
    isHostAutomatedChange = false;

    paramAdapter->clearFIFO();

    if (programs.hasCurrentProgram())
    {
        const Parameters &prog = programs.getCurrentProgram();
        for (auto *param : ObxfParams(*this))
        {
            if (param)
            {
                const auto &paramId = param->paramID;
                auto it = prog.values.find(paramId);
                const float value =
                    (it != prog.values.end()) ? it->second.load() : param->meta.defaultVal;

                const float normalized = param->convertTo0to1(value);

                param->beginChangeGesture();
                param->setValueNotifyingHost(normalized);
                param->endChangeGesture();
                paramAdapter->queue(paramId, normalized);
            }
        }
    }

    isHostAutomatedChange = true;
    sendChangeMessage();
    updateHostDisplay();
}

void ObxfAudioProcessor::setCurrentProgram(const int index, const bool updateHost)
{
    programs.currentProgram = index;
    isHostAutomatedChange = false;

    if (programs.hasCurrentProgram())
    {
        const Parameters &prog = programs.getCurrentProgram();
        for (auto *param : ObxfParams(*this))
        {
            if (param)
            {
                const auto &paramId = param->paramID;
                auto it = prog.values.find(paramId);
                const float value =
                    (it != prog.values.end()) ? it->second.load() : param->meta.defaultVal;

                const float normalized = param->convertTo0to1(value);

                param->beginChangeGesture();
                param->setValueNotifyingHost(normalized);
                param->endChangeGesture();
                paramAdapter->queue(paramId, normalized);
            }
        }
    }

    isHostAutomatedChange = true;

    sendChangeMessage();
    if (updateHost)
    {
        updateHostDisplay();
    }
}
const juce::String ObxfAudioProcessor::getProgramName(const int index)
{
    return programs.programs[index].getName();
}

void ObxfAudioProcessor::changeProgramName(const int index, const juce::String &newName)
{
    programs.programs[index].setName(newName);
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
    state->getStateInformation(destData);
}

void ObxfAudioProcessor::setStateInformation(const void *data, const int sizeInBytes)
{
    // TODO: NOT WORKING
    state->setStateInformation(data, sizeInBytes, true);
}

void ObxfAudioProcessor::initializeMidiCallbacks()
{
    midiHandler.onProgramChangeCallback = [this](const int programNumber) {
        onProgramChange(programNumber);
    };
}

void ObxfAudioProcessor::initializeUtilsCallbacks()
{
    utils->setHostUpdateCallback([this]() { updateHostDisplay(); });

    utils->loadMemoryBlockCallback = [this](juce::MemoryBlock &mb) {
        return loadFromMemoryBlock(mb);
    };

    utils->getStateInformationCallback = [this](juce::MemoryBlock &mb) { getStateInformation(mb); };

    utils->getNumProgramsCallback = [this]() { return getNumPrograms(); };

    utils->getCurrentProgramStateInformation = [this](juce::MemoryBlock &mb) {
        state->getCurrentProgramStateInformation(mb);
    };

    utils->getNumPrograms = [this]() { return getNumPrograms(); };

    utils->copyProgramNameToBuffer = [this](char *buffer, const int maxSize) {
        if (programs.hasCurrentProgram())
            programs.getCurrentProgram().getName().copyToUTF8(buffer, maxSize);
    };

    utils->setPatchName = [this](const juce::String &name) {
        if (programs.hasCurrentProgram())
            programs.getCurrentProgram().setName(name);
    };

    utils->resetPatchToDefault = [this]() {
        if (programs.hasCurrentProgram())
            programs.getCurrentProgram().setDefaultValues();
    };

    utils->sendChangeMessage = [this]() { sendChangeMessage(); };

    utils->setCurrentProgram = [this](const int index) { setCurrentProgram(index); };

    utils->isProgramNameCallback = [this](const int index, const juce::String &name) {
        return programs.programs[index].getName() == name;
    };
}

void ObxfAudioProcessor::initializeCallbacks()
{
    initializeMidiCallbacks();
    initializeUtilsCallbacks();
}

void ObxfAudioProcessor::randomizeAllPans()
{
    isHostAutomatedChange = false;
    std::uniform_real_distribution dist(-1.0f, 1.0f);
    for (auto *param : ObxfParams(*this))
    {
        if (param && param->meta.hasFeature(IS_PAN))
        {
            auto res = dist(panRng);
            // This smudges us a bit towards center pref
            res = res * res * res;
            res = (res + 1.0f) / 2.0f;

            param->beginChangeGesture();
            param->setValueNotifyingHost(res);
            param->endChangeGesture();

            paramAdapter->queue(param->paramID, res);
        }
    }

    isHostAutomatedChange = true;
    sendChangeMessage();
    updateHostDisplay();
}

void ObxfAudioProcessor::resetAllPansToDefault()
{
    isHostAutomatedChange = false;
    for (auto *param : ObxfParams(*this))
    {
        if (!param)
            continue;

        if (param->meta.hasFeature(IS_PAN))
        {
            constexpr float rawValue = 0.5f;
            param->beginChangeGesture();
            param->setValueNotifyingHost(rawValue);
            param->endChangeGesture();

            paramAdapter->queue(param->paramID, rawValue);
        }
    }
    isHostAutomatedChange = true;
    sendChangeMessage();
    updateHostDisplay();
}

void ObxfAudioProcessor::updateUIState()
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        uiState.voiceStatusValue[i] = synth.getVoiceStatus(i);
    }
}

//==============================================================================

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new ObxfAudioProcessor(); }
