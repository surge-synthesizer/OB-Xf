#include "PluginProcessor.h"
#include "Constants.h"
#include "PluginEditor.h"

ObxdAudioProcessor::ObxdAudioProcessor()
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
      midiHandler(synth, bindings, programs, *paramManager, *utils),
      state(std::make_unique<StateManager>(this))
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
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        apvts.addParameterListener(paramManager->getEngineParameterId(i), this);
    }

    apvts.state = juce::ValueTree(JucePlugin_Name);

    midiHandler.initMidi();
}
#endif

ObxdAudioProcessor::~ObxdAudioProcessor() = default;

void ObxdAudioProcessor::initAllParams()
{
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        paramManager->
            setEngineParameterValue(synth, i, programs.currentProgramPtr->values[i], true);
    }
}

void ObxdAudioProcessor::prepareToPlay(const double sampleRate, const int /*samplesPerBlock*/)
{
    midiHandler.prepareToPlay();
    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;

    midiMessageCollector.reset(sampleRate);

    paramManager->updateParameters(true);

    synth.setSampleRate(static_cast<float>(sampleRate));
}

void ObxdAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    midiMessageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

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
bool ObxdAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
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
void ObxdAudioProcessor::releaseResources()
{
}

const juce::String ObxdAudioProcessor::getName() const { return JucePlugin_Name; }

bool ObxdAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ObxdAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ObxdAudioProcessor::isMidiEffect() const { return false; }
double ObxdAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ObxdAudioProcessor::getNumPrograms() { return PROGRAMCOUNT; }
int ObxdAudioProcessor::getCurrentProgram() { return programs.currentProgram; }

void ObxdAudioProcessor::setCurrentProgram(const int index)
{
    programs.currentProgram = index;
    programs.currentProgramPtr = programs.programs + programs.currentProgram;
    isHostAutomatedChange = false;

    paramManager->clearFIFO();

    const auto &apvtState = paramManager->getValueTreeState();
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        auto paramId = paramManager->getEngineParameterId(i);
        if (auto *param = apvtState.getParameter(paramId))
        {
            param->setValueNotifyingHost(programs.currentProgramPtr->values[i]);
        }
    }

    isHostAutomatedChange = true;
    sendChangeMessage();
    updateHostDisplay();
}

void ObxdAudioProcessor::setCurrentProgram(const int index, const bool updateHost)
{
    programs.currentProgram = index;
    programs.currentProgramPtr = programs.programs + programs.currentProgram;
    isHostAutomatedChange = false;

    for (int i = 0; i < PARAM_COUNT; ++i)
        paramManager->
            setEngineParameterValue(synth, i, programs.currentProgramPtr->values[i], true);

    isHostAutomatedChange = true;

    sendChangeMessage();
    // Will delay
    if (updateHost)
    {
        updateHostDisplay();
    }
}

const juce::String ObxdAudioProcessor::getProgramName(const int index)
{
    return programs.programs[index].name;
}

void ObxdAudioProcessor::changeProgramName(const int index, const juce::String &newName)
{
    programs.programs[index].name = newName;
}

bool ObxdAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *ObxdAudioProcessor::createEditor()
{
    return new ObxdAudioProcessorEditor(*this);
}

void ObxdAudioProcessor::parameterChanged(const juce::String &parameterID, const float newValue)
{
    if (const int index = paramManager->getParameterIndexFromId(parameterID);
        juce::isPositiveAndBelow(index, PARAM_COUNT))
    {
        isHostAutomatedChange = false;
        paramManager->setEngineParameterValue(synth, index, newValue);
        isHostAutomatedChange = true;
    }
}

void ObxdAudioProcessor::setEngineParameterValue(int index, float newValue, bool notifyToHost)
{
    paramManager->setEngineParameterValue(synth, index, newValue, notifyToHost);
}

bool ObxdAudioProcessor::loadFromMemoryBlock(juce::MemoryBlock &mb) const
{
    return state->loadFromMemoryBlock(mb);
}

void ObxdAudioProcessor::onProgramChange(const int programNumber)
{
    setCurrentProgram(programNumber);
}

void ObxdAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    state->getStateInformation(destData);
}

void ObxdAudioProcessor::setStateInformation(const void *data, const int sizeInBytes)
{
    state->setStateInformation(data, sizeInBytes);
}

void ObxdAudioProcessor::initializeMidiCallbacks()
{
    midiHandler.onProgramChangeCallback = [this](const int programNumber) {
        onProgramChange(programNumber);
    };
}

void ObxdAudioProcessor::initializeUtilsCallbacks()
{
    utils->setHostUpdateCallback([this]() { updateHostDisplay(); });

    utils->loadMemoryBlockCallback = [this](juce::MemoryBlock &mb) {
        return loadFromMemoryBlock(mb);
    };

    utils->getStateInformationCallback = [this](juce::MemoryBlock &mb) {
        getStateInformation(mb);
    };

    utils->getNumProgramsCallback = [this]() { return getNumPrograms(); };

    utils->getCurrentProgramStateInformation = [this](juce::MemoryBlock &mb) {
        state->getCurrentProgramStateInformation(mb);
    };

    utils->getNumPrograms = [this]() { return getNumPrograms(); };

    utils->copyProgramNameToBuffer = [this](char *buffer, const int maxSize) {
        programs.currentProgramPtr->name.copyToUTF8(buffer, maxSize);
    };

    utils->setProgramName = [this](const juce::String &name) {
        programs.currentProgramPtr->name = name;
    };

    utils->resetProgramToDefault = [this]() {
        programs.currentProgramPtr->setDefaultValues();
    };

    utils->sendChangeMessage = [this]() { sendChangeMessage(); };

    utils->setCurrentProgram = [this](const int index) { setCurrentProgram(index); };

    utils->isProgramNameCallback = [this](const int index, const juce::String &name) {
        return programs.programs[index].name == name;
    };
}

void ObxdAudioProcessor::initializeCallbacks()
{
    initializeMidiCallbacks();
    initializeUtilsCallbacks();
}

//==============================================================================

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new ObxdAudioProcessor(); }