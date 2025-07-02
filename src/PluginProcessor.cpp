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
      paramManager(std::make_unique<ParameterManagerAdaptor>(*this, *this, *this)),
      midiHandler(synth, bindings, *paramManager, *utils),
      state(std::make_unique<StateManager>(this)), panRng(std::random_device{}())
{
    isHostAutomatedChange = true;

    synth.setSampleRate(44100);

    initializeCallbacks();

    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;

    initAllParams();

    auto &apvts = paramManager->getValueTreeState();
    apvts.state = juce::ValueTree(JucePlugin_Name);

    setCurrentProgram(0);

    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        apvts.addParameterListener(paramManager->getEngineParameterId(i), this);
    }

    midiHandler.initMidi();
}
#endif

ObxfAudioProcessor::~ObxfAudioProcessor() = default;

void ObxfAudioProcessor::initAllParams()
{
    if (const Parameters *prog = programs.currentProgramPtr.load())
    {
        for (int i = 0; i < PARAM_COUNT; ++i)
        {
            paramManager->setEngineParameterValue(synth, i, prog->values[i], true);
        }
    }
}

void ObxfAudioProcessor::prepareToPlay(const double sampleRate, const int /*samplesPerBlock*/)
{
    midiHandler.prepareToPlay();

    paramManager->updateParameters(true);

    synth.setSampleRate(static_cast<float>(sampleRate));
}

void ObxfAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    paramManager->updateParameters();

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
    programs.currentProgramPtr = programs.programs + programs.currentProgram;
    isHostAutomatedChange = false;

    paramManager->clearFIFO();

    if (const Parameters *prog = programs.currentProgramPtr.load())
    {
        const auto &apvtState = paramManager->getValueTreeState();
        for (int i = 0; i < PARAM_COUNT; ++i)
        {
            auto paramId = paramManager->getEngineParameterId(i);
            if (auto *param = apvtState.getParameter(paramId))
            {
                param->setValueNotifyingHost(prog->values[i]);
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
    programs.currentProgramPtr = programs.programs + programs.currentProgram;
    isHostAutomatedChange = false;

    if (const Parameters *prog = programs.currentProgramPtr.load())
    {
        for (int i = 0; i < PARAM_COUNT; ++i)
            paramManager->setEngineParameterValue(synth, i, prog->values[i], true);
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

void ObxfAudioProcessor::parameterChanged(const juce::String &parameterID, const float newValue)
{
    if (const int index = paramManager->getParameterIndexFromId(parameterID);
        juce::isPositiveAndBelow(index, PARAM_COUNT))
    {
        isHostAutomatedChange = false;
        paramManager->setEngineParameterValue(synth, index, newValue);
        isHostAutomatedChange = true;
    }
}

void ObxfAudioProcessor::setEngineParameterValue(int index, float newValue, bool notifyToHost)
{
    paramManager->setEngineParameterValue(synth, index, newValue, notifyToHost);
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
        if (const Parameters *prog = programs.currentProgramPtr.load())
            prog->getName().copyToUTF8(buffer, maxSize);
    };

    utils->setPatchName = [this](const juce::String &name) {
        if (Parameters *prog = programs.currentProgramPtr.load())
            prog->setName(name);
    };

    utils->resetPatchToDefault = [this]() {
        if (Parameters *prog = programs.currentProgramPtr.load())
            prog->setDefaultValues();
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
    std::uniform_real_distribution dist(0.0f, 1.0f);
    for (auto *param : getParameters())
    {
        if (param && param->getName(20).toLowerCase().contains("pan"))
        {
            param->setValueNotifyingHost(dist(panRng));
        }
    }
}

void ObxfAudioProcessor::resetAllPansToDefault() const
{
    for (auto *param : getParameters())
    {
        if (param && param->getName(20).toLowerCase().contains("pan"))
        {
            auto *floatParam = dynamic_cast<juce::AudioParameterFloat *>(param);

            const float normalized = floatParam->convertTo0to1(0.5f);
            floatParam->setValueNotifyingHost(normalized);
        }
    }
}

//==============================================================================

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new ObxfAudioProcessor(); }
