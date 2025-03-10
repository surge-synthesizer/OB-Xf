#include "PluginProcessor.h"
#include "Constants.h"
#include "PluginEditor.h"
//#include "Engine/Params.h"
#include "Utils.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::AudioParameterFloat>> params;

    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        const ObxdParams defaultParams;
        auto id           = ParameterManager::getEngineParameterId (i);
        auto name         = TRANS (id);
        auto range        = juce::NormalisableRange<float> {0.0f, 1.0f};
        auto defaultValue = defaultParams.values[i];
        auto parameter    = std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID{ id, 1 }, name, range, defaultValue, juce::String{}, juce::AudioProcessorParameter::genericParameter,
            [=](const float value, int /*maxStringLength*/)
            {
                return ParameterManager::getTrueParameterValueFromNormalizedRange(i, value);
            });

        params.push_back (std::move (parameter));
    }

    return { params.begin(), params.end() };
}

ObxdAudioProcessor::ObxdAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ), apvtState(*this, &undoManager, "PARAMETERS", createParameterLayout()),
        paramManager(*this, *this)
{
    isHostAutomatedChange = true;
    midiControlledParamSet = false;
    lastMovedController = 0;
    lastUsedParameter = 0;

    synth.setSampleRate(44100);

    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;
    initAllParams();

    for (int i = 0; i < PARAM_COUNT; ++i) {
        apvtState.addParameterListener(ParameterManager::getEngineParameterId(i), this);
    }

    apvtState.state = juce::ValueTree(JucePlugin_Name);
    //initMidi();
}
#endif
ObxdAudioProcessor::~ObxdAudioProcessor()
= default;

void ObxdAudioProcessor::initAllParams()
{
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        paramManager.setEngineParameterValue (synth, i, programs.currentProgramPtr->values[i], true);
    }
}

void ObxdAudioProcessor::prepareToPlay(const double sampleRate, const int /*samplesPerBlock*/)
{
    nextMidi = std::make_unique<juce::MidiMessage>(0xF0);
    midiMsg  = std::make_unique<juce::MidiMessage>(0xF0);

    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;

    synth.setSampleRate (sampleRate);
}

inline void ObxdAudioProcessor::processMidiPerSample(juce::MidiBufferIterator* iter, const juce::MidiBuffer& midiBuffer, const int samplePos)
{
    while (getNextEvent(iter, midiBuffer, samplePos))
    {
        if (!midiMsg)
            continue;

        const auto size = midiMsg->getRawDataSize();
        if (size < 1)
            continue;

        const auto* data = midiMsg->getRawData();
        if (!data)
            continue;

        const auto status = data[0] & 0xF0;
        if (status != 0x80 && status != 0x90 && status != 0xB0 &&
            status != 0xC0 && status != 0xE0)
            continue;

        DBG("Valid Message: " << (int)midiMsg->getChannel() << " "
            << (int)status << " "
            << (size > 1 ? (int)data[1] : 0) << " "
            << (size > 2 ? (int)data[2] : 0));

        if (midiMsg->isNoteOn())
        {
            synth.procNoteOn(midiMsg->getNoteNumber(), midiMsg->getFloatVelocity());
        }
        else if (midiMsg->isNoteOff())
        {
            synth.procNoteOff(midiMsg->getNoteNumber());
        }
        if (midiMsg->isPitchWheel())
        {
            synth.procPitchWheel((midiMsg->getPitchWheelValue() - 8192) / 8192.0f);
        }
        if (midiMsg->isController() && midiMsg->getControllerNumber() == 1)
        {
            synth.procModWheel(midiMsg->getControllerValue() / 127.0f);
        }
        if (midiMsg->isSustainPedalOff() || midiMsg->isAllNotesOff() || midiMsg->isAllSoundOff())
        {
            synth.sustainOff();
        }
        if (midiMsg->isAllNotesOff())
        {
            synth.allNotesOff();
        }
        if (midiMsg->isAllSoundOff())
        {
            synth.allSoundOff();
        }

        DBG(" Message: " << midiMsg->getChannel() << " "
            << " " << ((size > 2) ? midiMsg->getRawData()[2] : 0));

        if (midiMsg->isProgramChange())  // xC0
        {
            setCurrentProgram(midiMsg->getProgramChangeNumber());
        }
        else if (midiMsg->isController()) // xB0
        {
            lastMovedController = midiMsg->getControllerNumber();
            if (programs.currentProgramPtr->values[MIDILEARN] > 0.5f)
            {
                midiControlledParamSet = true;
                bindings.updateCC(lastUsedParameter, lastMovedController);
                juce::File midi_file = Utils::getMidiFolder().getChildFile("Custom.xml");
                bindings.saveFile(midi_file);
                currentMidiPath = midi_file.getFullPathName();

                paramManager.setEngineParameterValue(synth, MIDILEARN, 0, true);
                lastMovedController = 0;
                lastUsedParameter = 0;
                midiControlledParamSet = false;
            }

            if (bindings[lastMovedController] > 0)
            {
                midiControlledParamSet = true;
                paramManager.setEngineParameterValue(synth, bindings[lastMovedController],
                                        midiMsg->getControllerValue() / 127.0f, true);

                paramManager.setEngineParameterValue(synth, MIDILEARN, 0, true);
                lastMovedController = 0;
                lastUsedParameter = 0;
                midiControlledParamSet = false;
            }
        }
    }
}

bool ObxdAudioProcessor::getNextEvent(juce::MidiBufferIterator* iter, const juce::MidiBuffer& midiBuffer, const int samplePos)
{
	if (iter == nullptr)
		return false;

	if (*iter == midiBuffer.end())
		return false;

    if (const auto metadata = **iter; metadata.samplePosition >= samplePos && metadata.getMessage().getRawDataSize() > 0)
	{
		*midiMsg = metadata.getMessage();
		midiEventPos = metadata.samplePosition;
		++(*iter);
		return true;
	}
    else
	{
		++(*iter);
		return getNextEvent(iter, midiBuffer, samplePos);
	}
}

void ObxdAudioProcessor::initMidi(){
    //Documents > Obxd > MIDI > Default.xml
    if (juce::File default_file = Utils::getMidiFolder().getChildFile("Default.xml"); !default_file.exists()){
        bindings.saveFile(default_file);
    }

    const juce::File midi_config_file = Utils::getMidiFolder().getChildFile("Config.xml");
    juce::XmlDocument xmlDoc (midi_config_file);

    if (const std::unique_ptr<juce::XmlElement> ele_file = xmlDoc.getDocumentElementIfTagMatches("File")) {
        const juce::String file_name = ele_file->getStringAttribute("name");
        // Midi cc loading
        if (juce::File midi_file = Utils::getMidiFolder().getChildFile(file_name); bindings.loadFile(midi_file)){
            currentMidiPath = midi_file.getFullPathName();
        } else {
            if (juce::File xml = Utils::getMidiFolder().getChildFile("Default.xml"); bindings.loadFile(xml)){
                currentMidiPath = xml.getFullPathName();
            }
        }
    }
}


void ObxdAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    int samplePos = 0;
    const int numSamples = buffer.getNumSamples();
    float* channelData1 = buffer.getWritePointer(0);
    float* channelData2 = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : channelData1;

    if (const auto* playHead = getPlayHead())
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
    hasMidiMessage = (it != midiMessages.end());
    midiEventPos = 0;

    while (samplePos < numSamples)
    {
        processMidiPerSample(&it, midiMessages, samplePos);
        synth.processSample(channelData1 + samplePos, channelData2 + samplePos);
        ++samplePos;
    }
}




#ifndef JucePlugin_PreferredChannelConfigurations
bool ObxdAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif


    return true;
#endif
}
#endif

//==============================================================================
void ObxdAudioProcessor::releaseResources(){}
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

    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        auto paramId = ParameterManager::getEngineParameterId(i);
        if (auto* param = apvtState.getParameter(paramId))
        {
            param->setValueNotifyingHost(programs.currentProgramPtr->values[i]);
        }
        paramManager.setEngineParameterValue(synth, i, programs.currentProgramPtr->values[i], true);
    }

    isHostAutomatedChange = true;
    sendChangeMessage();
    updateHostDisplay();
}
const juce::String ObxdAudioProcessor::getProgramName(int index) { return programs.programs[index].name; }
void ObxdAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    programs.programs[index].name = newName;
}
bool ObxdAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* ObxdAudioProcessor::createEditor() {return new ObxdAudioProcessorEditor(*this);}
void ObxdAudioProcessor::parameterChanged(const juce::String &parameterID, const float newValue)
{
    int index = ParameterManager::getParameterIndexFromId (parameterID);

    if ( juce::isPositiveAndBelow (index, PARAM_COUNT) )
    {
        isHostAutomatedChange = false;
        paramManager.setEngineParameterValue(synth, index, newValue);
        isHostAutomatedChange = true;
    }
}


void ObxdAudioProcessor::getStateInformation (juce::MemoryBlock& /*destData*/){}
void ObxdAudioProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/){}
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new ObxdAudioProcessor();}


