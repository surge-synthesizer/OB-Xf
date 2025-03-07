/*
==============================================================================

This file was auto-generated!

It contains the basic startup code for a Juce application.

==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Engine/Params.h"

// Compare JUCE_VERSION against this to check for JUCE 5.4.3 and earlier
#define JUCE_543 328707

//only sse2 version on windows
#ifdef _WINDOWS
#define __SSE2__
#define __SSE__
#endif

#ifdef __SSE2__
#include <xmmintrin.h>

#include <memory>
#endif

//==============================================================================
#define S(T) (juce::String(T))

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    ObxdParams defaultParams;
    std::vector<std::unique_ptr<AudioParameterFloat>> params;
    
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        auto id           = ObxdAudioProcessor::getEngineParameterId (i);
        auto name         = TRANS (id);
        auto range        = NormalisableRange<float> {0.0f, 1.0f};
        auto defaultValue = defaultParams.values[i];
        auto parameter    = std::make_unique<AudioParameterFloat> (
            ParameterID{ id, 1 }, name, range, defaultValue, String{}, AudioProcessorParameter::genericParameter,
            [=](float value, int /*maxStringLength*/)
            {
                return ObxdAudioProcessor::getTrueParameterValueFromNormalizedRange(i, value);
            });
        
        params.push_back (std::move (parameter));
    }
    
    return { params.begin(), params.end() };
}

//==============================================================================
ObxdAudioProcessor::ObxdAudioProcessor()
	: programs()
	, bindings()
	, configLock("__" JucePlugin_Name "ConfigLock__")
	, apvtState (*this, &undoManager, "PARAMETERS", createParameterLayout())
{
	isHostAutomatedChange = true;
	midiControlledParamSet = false;
	lastMovedController = 0;
	lastUsedParameter = 0;

	synth.setSampleRate (44100);

	PropertiesFile::Options options;
	options.applicationName = JucePlugin_Name;
	options.storageFormat = PropertiesFile::storeAsXML;
	options.millisecondsBeforeSaving = 2500;
	options.processLock = &configLock;
	config = std::make_unique<PropertiesFile>(getDocumentFolder().getChildFile("Skin.xml"), options);
	showPresetBar = config->getBoolValue("presetnavigation");
	gui_size = config->getIntValue("gui_size", 1);
	tooltipBehavior = static_cast<Tooltip>(config->getIntValue("tooltip", 1));
	currentSkin = config->containsKey("skin") ? config->getValue("skin") : "Ilkka Rosma Dark";
	currentBank = "000 - FMR OB-Xa Patch Book";

	scanAndUpdateBanks();
	scanAndUpdateSkins();
	initAllParams();

	if (bankFiles.size() > 0)
	{
		loadFromFXBFile (bankFiles[0]);
	}

	for (int i = 0; i < PARAM_COUNT; ++i)
	{
		apvtState.addParameterListener (getEngineParameterId (i), this);
	}

	apvtState.state = ValueTree (JucePlugin_Name);
	initMidi();
}

ObxdAudioProcessor::~ObxdAudioProcessor()
{
    
	config->saveIfNeeded();
	config = nullptr;
}

//==============================================================================
void ObxdAudioProcessor::initAllParams()
{
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        setEngineParameterValue (i, programs.currentProgramPtr->values[i], true);
    }
}

//==============================================================================
const String ObxdAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

const String ObxdAudioProcessor::getInputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

const String ObxdAudioProcessor::getOutputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

bool ObxdAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
	return true;
}

bool ObxdAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
	return true;
}

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

double ObxdAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

//==============================================================================
int ObxdAudioProcessor::getNumPrograms()
{
	return PROGRAMCOUNT;
}

int ObxdAudioProcessor::getCurrentProgram()
{
	return programs.currentProgram;
}

void ObxdAudioProcessor::setCurrentProgram (int index, bool updateHost){
    programs.currentProgram = index;
    programs.currentProgramPtr = programs.programs + programs.currentProgram;
    isHostAutomatedChange = false;
    
    for (int i = 0; i < PARAM_COUNT; ++i)
        setEngineParameterValue (i, programs.currentProgramPtr->values[i], true);
    
    isHostAutomatedChange = true;
    
    sendChangeMessage();
    // Will delay
    if (updateHost) {
        updateHostDisplay();
    }
}

void ObxdAudioProcessor::setCurrentProgram (int index)
{
	programs.currentProgram = index;
	programs.currentProgramPtr = programs.programs + programs.currentProgram;
	isHostAutomatedChange = false;
    
	for (int i = 0; i < PARAM_COUNT; ++i)
		setEngineParameterValue (i, programs.currentProgramPtr->values[i], true);
    
	isHostAutomatedChange = true;
	sendChangeMessage();
	updateHostDisplay();
}

const String ObxdAudioProcessor::getProgramName (int index)
{
	return programs.programs[index].name;
}

void ObxdAudioProcessor::changeProgramName (int index, const String& newName)
{
	 programs.programs[index].name = newName;
}

//==============================================================================
void ObxdAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	nextMidi = new MidiMessage (0xF0);
	midiMsg  = new MidiMessage (0xF0);
	synth.setSampleRate (sampleRate);
}

void ObxdAudioProcessor::releaseResources()
{
}

inline void ObxdAudioProcessor::processMidiPerSample (MidiBufferIterator* iter, const int samplePos)
{
	while (getNextEvent (iter, samplePos))
	{
		if (midiMsg->isNoteOn())
		{
			synth.procNoteOn (midiMsg->getNoteNumber(), midiMsg->getFloatVelocity());
		}
		if (midiMsg->isNoteOff())
		{
			synth.procNoteOff (midiMsg->getNoteNumber());
		}
		if (midiMsg->isPitchWheel())
		{
			// [0..16383] center = 8192;
			synth.procPitchWheel ((midiMsg->getPitchWheelValue() - 8192) / 8192.0f);
		}
		if (midiMsg->isController() && midiMsg->getControllerNumber() == 1)
        {
			synth.procModWheel (midiMsg->getControllerValue() / 127.0f);
        }
		if(midiMsg->isSustainPedalOn())
		{
			synth.sustainOn();
		}
		if(midiMsg->isSustainPedalOff() || midiMsg->isAllNotesOff()||midiMsg->isAllSoundOff())
		{
			synth.sustainOff();
		}
		if(midiMsg->isAllNotesOff())
		{
			synth.allNotesOff();
		}
		if(midiMsg->isAllSoundOff())
		{
			synth.allSoundOff();
        }
        
        DBG(" Message: " << midiMsg->getChannel() << " "<<midiMsg->getRawData()[0] << " "<< midiMsg->getRawData()[1] << " "<< midiMsg->getRawData()[2]);
        
        if (midiMsg->isProgramChange()){ // xC0
            setCurrentProgram(midiMsg->getProgramChangeNumber());
            
        } else
        if (midiMsg->isController()) // xB0
        {
            lastMovedController = midiMsg->getControllerNumber();
            if (programs.currentProgramPtr->values[MIDILEARN] > 0.5f){
                midiControlledParamSet = true;
                //bindings[lastMovedController] = lastUsedParameter;
                bindings.updateCC(lastUsedParameter, lastMovedController);
                File midi_file = getMidiFolder().getChildFile("Custom.xml");
                bindings.saveFile(midi_file);
                currentMidiPath = midi_file.getFullPathName();
                
                setEngineParameterValue (MIDILEARN, 0, true);
                lastMovedController = 0;
                lastUsedParameter = 0;
                midiControlledParamSet = false;
                
                
            }

            if (bindings[lastMovedController] > 0)
            {
                midiControlledParamSet = true;
                setEngineParameterValue (bindings[lastMovedController],
                                         midiMsg->getControllerValue() / 127.0f, true);
                
                setEngineParameterValue (MIDILEARN, 0, true);
                lastMovedController = 0;
                lastUsedParameter = 0;

                midiControlledParamSet = false;
            }
        }
	}
}

bool ObxdAudioProcessor::getNextEvent(juce::MidiBufferIterator* iter, const int samplePos)
{
	//potential issues here
	if (hasMidiMessage && midiEventPos <= samplePos)
	{
		auto metadata = **iter;
		*midiMsg = metadata.getMessage();
		midiEventPos = metadata.samplePosition;
		++(*iter);
		hasMidiMessage = (iter != nullptr);
		return true;
	}

	return false;
}

// void ObxdAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
// {
// 	//SSE flags set
// #ifdef __SSE__
// 	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
// #endif
// #ifdef __SSE2__
// 	// _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
// #endif
//
// 	MidiBuffer::Iterator ppp (midiMessages);
// 	hasMidiMessage = ppp.getNextEvent (*nextMidi, midiEventPos);
//
// 	int samplePos = 0;
// 	int numSamples = buffer.getNumSamples();
// 	float* channelData1 = buffer.getWritePointer (0);
// 	float* channelData2 = buffer.getWritePointer (1);
//
// 	AudioPlayHead::CurrentPositionInfo pos;
//
//     if (getPlayHead() != 0 && getPlayHead()->getCurrentPosition (pos))
//     {
// 		synth.setPlayHead(pos.bpm, pos.ppqPosition);
//     }
//
// 	//maybe right?
// 	auto it = midiMessages.findNextSamplePosition(0);
// 	hasMidiMessage = true;
// 	midiEventPos = 0;
//
// 	while (samplePos < numSamples)
// 	{
// 		processMidiPerSample (&it, samplePos);
// 		synth.processSample (channelData1 + samplePos, channelData2 + samplePos);
// 		++samplePos;
// 	}
// }

void ObxdAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	// SSE flags set
#ifdef __SSE__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif
#ifdef __SSE2__
	// _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	int samplePos = 0;
	int numSamples = buffer.getNumSamples();
	float* channelData1 = buffer.getWritePointer(0);
	float* channelData2 = buffer.getWritePointer(1);

	// Update playhead using modern API
	if (auto* playHead = getPlayHead())
	{
		if (auto position = playHead->getPosition())
		{
			if (position->getBpm() && position->getPpqPosition())
			{
				synth.setPlayHead(*(position->getBpm()), *(position->getPpqPosition()));
			}
		}
	}

	// Initialize MIDI iterator using modern JUCE API
	auto it = midiMessages.begin();
	hasMidiMessage = (it != midiMessages.end());
	midiEventPos = 0;

	while (samplePos < numSamples)
	{
		processMidiPerSample(&it, samplePos);
		synth.processSample(channelData1 + samplePos, channelData2 + samplePos);
		++samplePos;
	}
}
//==============================================================================
bool ObxdAudioProcessor::hasEditor() const
{
	return true;
}

AudioProcessorEditor* ObxdAudioProcessor::createEditor()
{
	return new ObxdAudioProcessorEditor (*this);
}

//==============================================================================
void ObxdAudioProcessor::getStateInformation(MemoryBlock& destData)
{
	XmlElement xmlState = XmlElement("discoDSP");
	xmlState.setAttribute(S("currentProgram"), programs.currentProgram);

	XmlElement* xprogs = new XmlElement("programs");
	for (int i = 0; i < PROGRAMCOUNT; ++i)
	{
		XmlElement* xpr = new XmlElement("program");
		xpr->setAttribute(S("programName"), programs.programs[i].name);
		xpr->setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);

		for (int k = 0; k < PARAM_COUNT; ++k)
		{
            xpr->setAttribute("Val_" + String(k), programs.programs[i].values[k]);
		}

		xprogs->addChildElement(xpr);
	}

	xmlState.addChildElement(xprogs);

    //bindings.setXml(xmlState);

	copyXmlToBinary(xmlState, destData);
}

void ObxdAudioProcessor::getCurrentProgramStateInformation(MemoryBlock& destData)
{
	XmlElement xmlState = XmlElement("discoDSP");

	for (int k = 0; k < PARAM_COUNT; ++k)
	{
		xmlState.setAttribute("Val_" + String(k), programs.currentProgramPtr->values[k]);
	}

	xmlState.setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);
	xmlState.setAttribute(S("programName"), programs.currentProgramPtr->name);

	copyXmlToBinary(xmlState, destData);
}

void ObxdAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
#if JUCE_VERSION <= JUCE_543
	XmlElement * const xmlState = getXmlFromBinary(data, sizeInBytes);
#else
	std::unique_ptr<XmlElement> xmlState = getXmlFromBinary(data, sizeInBytes);
#endif
    
    DBG(" XML:" << xmlState->toString());
	if (xmlState)
	{
		XmlElement* xprogs = xmlState->getFirstChildElement();
		if (xprogs && xprogs->hasTagName(S("programs")))
		{
			int i = 0;
			// forEachXmlChildElement(*xprogs, e)
			// {
			// 	bool newFormat = e->hasAttribute("voiceCount");
			// 	programs.programs[i].setDefaultValues();
   //
			// 	for (int k = 0; k < PARAM_COUNT; ++k)
			// 	{
   //                  float value = 0.0;
   //                  if (e->hasAttribute("Val_" + String(k))){
   //                      value = float(e->getDoubleAttribute("Val_" + String(k), programs.programs[i].values[k]));
   //                  } else {
   //                      value = float(e->getDoubleAttribute(String(k), programs.programs[i].values[k]));
   //                  }
   //
			// 		if (!newFormat && k == VOICE_COUNT) value *= 0.25f;
			// 		programs.programs[i].values[k] = value;
			// 	}
			for (auto* e : xprogs->getChildIterator())
			{
				bool newFormat = e->hasAttribute("voiceCount");
				programs.programs[i].setDefaultValues();

				for (int k = 0; k < PARAM_COUNT; ++k)
				{
					float value = 0.0;
					if (e->hasAttribute("Val_" + String(k)))
					{
						value = float(e->getDoubleAttribute("Val_" + String(k), programs.programs[i].values[k]));
					}
					else
					{
						value = float(e->getDoubleAttribute(String(k), programs.programs[i].values[k]));
					}

					if (!newFormat && k == VOICE_COUNT) value *= 0.25f;
					programs.programs[i].values[k] = value;
				}

				programs.programs[i].name = e->getStringAttribute(S("programName"), S("Default"));

				++i;
				}
			}

		//bindings.getXml(*xmlState);
#if ! DEMOVERSION
		setCurrentProgram(xmlState->getIntAttribute(S("currentProgram"), 0));

        sendChangeMessage();
#endif
#if JUCE_VERSION <= JUCE_543
		delete xmlState;
#endif
		}
	}

void  ObxdAudioProcessor::setCurrentProgramStateInformation(const void* data, int sizeInBytes)
{
#if JUCE_VERSION <= JUCE_543
	XmlElement * const e = getXmlFromBinary(data, sizeInBytes);
#else
	std::unique_ptr<XmlElement> e = getXmlFromBinary(data, sizeInBytes);
#endif
	if (e)
	{
		programs.currentProgramPtr->setDefaultValues();

		bool newFormat = e->hasAttribute("voiceCount");
		for (int k = 0; k < PARAM_COUNT; ++k)
		{
            float value = 0.0 ;
            if (e->hasAttribute("Val_" + String(k))){
                value = float(e->getDoubleAttribute("Val_" + String(k), programs.currentProgramPtr->values[k]));
            } else {
                value = float(e->getDoubleAttribute(String(k), programs.currentProgramPtr->values[k]));
            }
			 
			if (!newFormat && k == VOICE_COUNT) value *= 0.25f;
			programs.currentProgramPtr->values[k] = value;
		}

		programs.currentProgramPtr->name = e->getStringAttribute(S("programName"), S("Default"));

		setCurrentProgram(programs.currentProgram);
        
        sendChangeMessage();
#if JUCE_VERSION <= JUCE_543
		delete e;
#endif
	}
}

//==============================================================================
bool ObxdAudioProcessor::deleteBank() {
    currentBankFile.deleteFile();
    scanAndUpdateBanks();
    if (bankFiles.size() > 0)
    {
        loadFromFXBFile (bankFiles[0]);
    }
    return true;
}

void ObxdAudioProcessor::saveBank() {
    saveFXBFile(currentBankFile);
}

bool ObxdAudioProcessor::loadPreset(const File& fxpFile) {
    loadFromFXPFile(fxpFile);
    currentPreset = fxpFile.getFileName();
    currentPresetFile = fxpFile;
    return true;
}

/// Serialize the current Preset, just like the saveFXPFile function,
/// but keeps the data in memory instead of writing it to a file.
void ObxdAudioProcessor::serializePreset(MemoryBlock& memoryBlock)
{
	juce::MemoryBlock m;
	getCurrentProgramStateInformation(m);
	{
		memoryBlock.reset();
		auto totalLen = sizeof (fxProgramSet) + m.getSize() - 8;
		memoryBlock.setSize (totalLen, true);

		auto set = static_cast<fxProgramSet*>(memoryBlock.getData());
		set->chunkMagic = fxbName ("CcnK");
		set->byteSize = 0;
		set->fxMagic = fxbName ("FPCh");
		set->version = fxbSwap (fxbVersionNum);
		set->fxID = fxbName ("Obxd");
		set->fxVersion = fxbSwap (fxbVersionNum);
		set->numPrograms = fxbSwap (getNumPrograms());
		programs.currentProgramPtr->name.copyToUTF8(set->name, 28);
		set->chunkSize = fxbSwap (static_cast<int32>(m.getSize()));

		m.copyTo (set->chunk, 0, m.getSize());
	}
}

bool ObxdAudioProcessor::saveFXPFile(const File& fxpFile){
    //auto xml = std::unique_ptr<juce::XmlElement>(new juce::XmlElement(""));
    juce::MemoryBlock m, memoryBlock;
    getCurrentProgramStateInformation(m);
    {
        memoryBlock.reset();
        auto totalLen = sizeof (fxProgramSet) + m.getSize() - 8;
        memoryBlock.setSize (totalLen, true);

        auto set = static_cast<fxProgramSet*>(memoryBlock.getData());
        set->chunkMagic = fxbName ("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName ("FPCh");
        set->version = fxbSwap (fxbVersionNum);
        set->fxID = fxbName ("Obxd");
        set->fxVersion = fxbSwap (fxbVersionNum);
        set->numPrograms = fxbSwap (getNumPrograms());
        programs.currentProgramPtr->name.copyToUTF8(set->name, 28);
        set->chunkSize = fxbSwap (static_cast<int32>(m.getSize()));

        m.copyTo (set->chunk, 0, m.getSize());
        
        fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }
    return true;
}

bool ObxdAudioProcessor::savePreset(const File& fxpFile) {
    saveFXPFile(fxpFile);
    currentPreset = fxpFile.getFileName();
    currentPresetFile = fxpFile;
    return true;
}

void ObxdAudioProcessor::changePresetName(const String &name){
    programs.currentProgramPtr->name = name;
    //savePreset();
    //saveBank();
}

void ObxdAudioProcessor::deletePreset(){
    programs.currentProgramPtr->setDefaultValues();
    programs.currentProgramPtr->name = "Default";
    sendChangeMessage();
    //saveBank();
}

void ObxdAudioProcessor::newPreset(const String &/*name*/) {
    for (int i = 0; i < PROGRAMCOUNT; ++i)
    {
        if (programs.programs[i].name == "Default"){
            setCurrentProgram(i);
            break;
        }
    }
    //savePreset();
    //saveBank();
}

void ObxdAudioProcessor::savePreset() {
    savePreset(currentPresetFile);
    
}

bool ObxdAudioProcessor::saveBank(const File& fxbFile){
    saveFXBFile(fxbFile);
    currentBankFile = fxbFile;
    return true;
}

bool ObxdAudioProcessor::saveFXBFile(const File& fxbFile) {
    //auto xml = std::unique_ptr<juce::XmlElement>(new juce::XmlElement(""));
    juce::MemoryBlock m, memoryBlock;
    getStateInformation(m);

    {
        memoryBlock.reset();
        auto totalLen = sizeof (fxChunkSet) + m.getSize() - 8;
        memoryBlock.setSize (totalLen, true);

        auto set = static_cast<fxChunkSet*>( memoryBlock.getData());
        set->chunkMagic = fxbName ("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName ("FBCh");
        set->version = fxbSwap (fxbVersionNum);
        set->fxID = fxbName ("Obxd");
        set->fxVersion = fxbSwap (fxbVersionNum);
        set->numPrograms = fxbSwap (getNumPrograms());
        set->chunkSize = fxbSwap (static_cast<int32>(m.getSize()));

        m.copyTo (set->chunk, 0, m.getSize());
        fxbFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }
    
    
    return true;
}

bool ObxdAudioProcessor::loadFromFXPFile(const File& fxpFile)
{
	MemoryBlock mb;
	if (! fxpFile.loadFileAsData(mb))
		return false;
	
	if (!loadFromMemoryBlock(mb))
		return false;

	currentPreset = fxpFile.getFileName();
	currentPresetFile = fxpFile;
	updateHostDisplay();
	
	return true;
}

bool ObxdAudioProcessor::loadFromFXBFile(const File& fxbFile)
{
	MemoryBlock mb;
	if (! fxbFile.loadFileAsData(mb))
		return false;
	
	if (!loadFromMemoryBlock(mb))
		return false;
	
	currentBank = fxbFile.getFileName();
	currentBankFile = fxbFile;
	updateHostDisplay();
	
	return true;
}

bool ObxdAudioProcessor::isMemoryBlockAPreset(const MemoryBlock& mb)
{
    const void* const data = mb.getData();
    const size_t dataSize = mb.getSize();

    if (dataSize < 28)
        return false;

    const fxSet* const set = (const fxSet*)data;

    if ((!compareMagic(set->chunkMagic, "CcnK")) || fxbSwap(set->version) > fxbVersionNum)
        return false;
    return true;
}

bool ObxdAudioProcessor::loadFromMemoryBlock(MemoryBlock& mb)
{
	const void* const data = mb.getData();
	const size_t dataSize = mb.getSize();

	if (dataSize < 28)
		return false;

	const fxSet* const set = (const fxSet*) data;

	if ((! compareMagic (set->chunkMagic, "CcnK")) || fxbSwap (set->version) > fxbVersionNum)
		return false;

	if (compareMagic (set->fxMagic, "FxBk"))
	{
		// bank of programs
		if (fxbSwap (set->numPrograms) >= 0)
		{
			const int oldProg = getCurrentProgram();
			const int numParams = fxbSwap (((const fxProgram*) (set->programs))->numParams);
			const int progLen = (int) sizeof (fxProgram) + (numParams - 1) * (int) sizeof (float);

			for (int i = 0; i < fxbSwap (set->numPrograms); ++i)
			{
				if (i != oldProg)
				{
					const fxProgram* const prog = (const fxProgram*) (((const char*) (set->programs)) + i * progLen);
					if (((const char*) prog) - ((const char*) set) >= (ssize_t) dataSize)
						return false;

					if (fxbSwap (set->numPrograms) > 0)
						setCurrentProgram (i);

					if (! restoreProgramSettings (prog))
						return false;
				}
			}

			if (fxbSwap (set->numPrograms) > 0)
				setCurrentProgram (oldProg);

			const fxProgram* const prog = (const fxProgram*) (((const char*) (set->programs)) + oldProg * progLen);
			if (((const char*) prog) - ((const char*) set) >= (ssize_t) dataSize)
				return false;

			if (! restoreProgramSettings (prog))
				return false;
		}
	}
	else if (compareMagic (set->fxMagic, "FxCk"))
	{
		// single program
		const fxProgram* const prog = (const fxProgram*) data;

		if (! compareMagic (prog->chunkMagic, "CcnK"))
			return false;

		changeProgramName (getCurrentProgram(), prog->prgName);

		for (int i = 0; i < fxbSwap (prog->numParams); ++i)
			setEngineParameterValue (i, fxbSwapFloat (prog->params[i]));
	}
	else if (compareMagic (set->fxMagic, "FBCh"))
	{
		// non-preset chunk
		const fxChunkSet* const cset = (const fxChunkSet*) data;

		if ((size_t) fxbSwap (cset->chunkSize) + sizeof (fxChunkSet) - 8 > (size_t) dataSize)
			return false;

		setStateInformation(cset->chunk, fxbSwap (cset->chunkSize));
        setCurrentProgram(0); // Set to first preset position
	}
	else if (compareMagic (set->fxMagic, "FPCh"))
	{
		// preset chunk
		const fxProgramSet* const cset = (const fxProgramSet*) data;

		if ((size_t) fxbSwap (cset->chunkSize) + sizeof (fxProgramSet) - 8 > (size_t) dataSize)
			return false;

		setCurrentProgramStateInformation(cset->chunk, fxbSwap (cset->chunkSize));

		changeProgramName (getCurrentProgram(), cset->name);
	}
	else
	{
		return false;
	}

	return true;
}

bool ObxdAudioProcessor::restoreProgramSettings(const fxProgram* const prog)
{
	if (compareMagic (prog->chunkMagic, "CcnK")
		&& compareMagic (prog->fxMagic, "FxCk"))
	{
		changeProgramName (getCurrentProgram(), prog->prgName);

		for (int i = 0; i < fxbSwap (prog->numParams); ++i)
			setEngineParameterValue (i, fxbSwapFloat (prog->params[i]));

		return true;
	}

	return false;
}

//==============================================================================
void ObxdAudioProcessor::scanAndUpdateBanks()
{
	bankFiles.clear();

	for (const auto& entry : RangedDirectoryIterator(getBanksFolder(), false, "*.fxb", File::findFiles))
	{
		bankFiles.addUsingDefaultSort(entry.getFile());
		DBG("Scan Banks: " << entry.getFile().getFullPathName());
	}
}

void ObxdAudioProcessor::scanAndUpdateSkins()
{
	skinFiles.clearQuick();

	for (const auto& entry : RangedDirectoryIterator(getSkinFolder(), false, "*", File::findDirectories))
	{
		skinFiles.addUsingDefaultSort(entry.getFile());
	}
}

const Array<File>& ObxdAudioProcessor::getBankFiles() const
{
	return bankFiles;
}

const Array<File>& ObxdAudioProcessor::getSkinFiles() const
{
    return skinFiles;
}

File ObxdAudioProcessor::getCurrentBankFile() const
{
	return getBanksFolder().getChildFile(currentBank);
}

//==============================================================================
File ObxdAudioProcessor::getDocumentFolder() const
{
	File folder = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("discoDSP").getChildFile("OB-Xd");
/*
    if (! folder.exists())
    {
        NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error", "Documents > discoDSP > OB-Xd folder not found.");
    }
 */
	if (folder.isSymbolicLink())
		folder = folder.getLinkedTarget();
	return folder;
}

File ObxdAudioProcessor::getSkinFolder() const
{
	return getDocumentFolder().getChildFile("Themes");
}

File ObxdAudioProcessor::getBanksFolder() const
{
	return getDocumentFolder().getChildFile("Banks");
}

File ObxdAudioProcessor::getMidiFolder() const
{
    return getDocumentFolder().getChildFile("MIDI");
}


File ObxdAudioProcessor::getPresetsFolder() const
{
    return getDocumentFolder().getChildFile("Presets");
}

File ObxdAudioProcessor::getCurrentSkinFolder() const
{
    DBG(" SKIN : " << currentSkin);
	return getSkinFolder().getChildFile(currentSkin);
}

void ObxdAudioProcessor::setCurrentSkinFolder(const String& folderName)
{
	currentSkin = folderName;

	config->setValue("skin", folderName);
	config->setNeedsToBeSaved(true);
}

void ObxdAudioProcessor::setGuiSize(const int gui_size) {
    this->gui_size = gui_size;
    config->setValue("gui_size", gui_size);
    config->setNeedsToBeSaved(true);
}

Tooltip ObxdAudioProcessor::getTooltipBehavior() const
{
    return tooltipBehavior;
}

void ObxdAudioProcessor::setTooltipBehavior(const Tooltip tooltip)
{
    this->tooltipBehavior = tooltip;
    config->setValue("tooltip", static_cast<int>(tooltip));
    config->setNeedsToBeSaved(true);
}

//==============================================================================
String ObxdAudioProcessor::getEngineParameterId (size_t index)
{
    switch (index)
	{
        case SELF_OSC_PUSH:      return "SelfOscPush";
        case ENV_PITCH_BOTH:     return "EnvPitchBoth";
        case FENV_INVERT:        return "FenvInvert";
		case PW_OSC2_OFS:        return "PwOfs";
        case LEVEL_DIF:          return "LevelDif";
        case PW_ENV_BOTH:        return "PwEnvBoth";
		case PW_ENV:             return "PwEnv";
	    case LFO_SYNC:           return "LfoSync";
        case ECONOMY_MODE:       return "EconomyMode";
	    case UNLEARN:            return "MidiUnlearn";
        case MIDILEARN:          return "MidiLearn";
	    case VAMPENV:            return "VAmpFactor";
        case VFLTENV:            return "VFltFactor";
	    case ASPLAYEDALLOCATION: return "AsPlayedAllocation";
	    case BENDLFORATE:        return "VibratoRate";
	    case FOURPOLE:           return "FourPole";
	    case LEGATOMODE:         return "LegatoMode";
	    case ENVPITCH:           return "EnvelopeToPitch";
	    case OSCQuantize:        return "PitchQuant";
        case VOICE_COUNT:        return "VoiceCount";
	    case BANDPASS:           return "BandpassBlend";
	    case FILTER_WARM:        return "Filter_Warm";
	    case BENDRANGE:          return "BendRange";
        case BENDOSC2:           return "BendOsc2Only";
        case OCTAVE:             return "Octave";
        case TUNE:               return "Tune";
        case BRIGHTNESS:         return "Brightness";
        case NOISEMIX:           return "NoiseMix";
        case OSC1MIX:            return "Osc1Mix";
        case OSC2MIX:            return "Osc2Mix";
        case MULTIMODE:          return "Multimode";
        case LFOSHWAVE:          return "LfoSampleHoldWave";
        case LFOSINWAVE:         return "LfoSineWave";
        case LFOSQUAREWAVE:      return "LfoSquareWave";
        case LFO1AMT:            return "LfoAmount1";
        case LFO2AMT:            return "LfoAmount2";
        case LFOFILTER:          return "LfoFilter";
        case LFOOSC1:            return "LfoOsc1";
        case LFOOSC2:            return "LfoOsc2";
        case LFOFREQ:            return "LfoFrequency";
        case LFOPW1:             return "LfoPw1";
        case LFOPW2:             return "LfoPw2";
        case PORTADER:           return "PortamentoDetune";
        case FILTERDER:          return "FilterDetune";
        case ENVDER:             return "EnvelopeDetune";
        case PAN1:               return "Pan1";
        case PAN2:               return "Pan2";
        case PAN3:               return "Pan3";
        case PAN4:               return "Pan4";
        case PAN5:               return "Pan5";
        case PAN6:               return "Pan6";
        case PAN7:               return "Pan7";
        case PAN8:               return "Pan8";
        case XMOD:               return "Xmod";
        case OSC2HS:             return "Osc2HardSync";
        case OSC1P:              return "Osc1Pitch";
        case OSC2P:              return "Osc2Pitch";
        case PORTAMENTO:         return "Portamento";
        case UNISON:             return "Unison";
        case FLT_KF:             return "FilterKeyFollow";
        case PW:                 return "PulseWidth";
        case OSC2Saw:            return "Osc2Saw";
        case OSC1Saw:            return "Osc1Saw";
        case OSC1Pul:            return "Osc1Pulse";
        case OSC2Pul:            return "Osc2Pulse";
        case VOLUME:             return "Volume";
        case UDET:               return "VoiceDetune";
        case OSC2_DET:           return "Oscillator2detune";
        case CUTOFF:             return "Cutoff";
        case RESONANCE:          return "Resonance";
        case ENVELOPE_AMT:       return "FilterEnvAmount";
        case LATK:               return "Attack";
        case LDEC:               return "Decay";
        case LSUS:               return "Sustain";
        case LREL:               return "Release";
        case FATK:               return "FilterAttack";
        case FDEC:               return "FilterDecay";
        case FSUS:               return "FilterSustain";
        case FREL:               return "FilterRelease";
            
        default:
            break;
    }
    
    return "Undefined";
}

String ObxdAudioProcessor::getTrueParameterValueFromNormalizedRange(size_t index, float value)
{
    switch (index)
    {
    // case SELF_OSC_PUSH:      return "SelfOscPush";
    // case ENV_PITCH_BOTH:     return "EnvPitchBoth";
    // case FENV_INVERT:        return "FenvInvert";
    // case PW_OSC2_OFS:        return "PwOfs";
    // case LEVEL_DIF:          return "LevelDif";
    // case PW_ENV_BOTH:        return "PwEnvBoth";
    // case PW_ENV:             return "PwEnv";
    // case LFO_SYNC:           return "LfoSync";
    // case ECONOMY_MODE:       return "EconomyMode";
    // case UNLEARN:            return "MidiUnlearn";
    // case MIDILEARN:          return "MidiLearn";
    // case VAMPENV:            return "VAmpFactor";
    // case VFLTENV:            return "VFltFactor";
    // case ASPLAYEDALLOCATION: return "AsPlayedAllocation";
    case BENDLFORATE:        return String{ logsc(value, 3, 10), 2 } + " Hz";
    // case FOURPOLE:           return "FourPole";
    // case LEGATOMODE:         return "LegatoMode";
    // case ENVPITCH:           return "EnvelopeToPitch";
    // case OSCQuantize:        return "PitchQuant";
    // case VOICE_COUNT:        return "VoiceCount";
    // case BANDPASS:           return "BandpassBlend";
    // case FILTER_WARM:        return "Filter_Warm";
    // case BENDRANGE:          return "BendRange";
    // case BENDOSC2:           return "BendOsc2Only";
    case OCTAVE:             return String{ (roundToInt(value * 4) - 2) * 12.f, 0 } + " Semitones";
    case TUNE:               return String{ value * 200 - 100, 1 } + " Cents";
    // case BRIGHTNESS:         return "Brightness";
    case NOISEMIX: {
        const auto decibels = Decibels::gainToDecibels(logsc(value, 0, 1, 35));
        if (decibels < -80) return "-Inf";
        return String{ decibels, 2 } + " dB";
    }
    case OSC1MIX:            
    case OSC2MIX: {
        const auto decibels = Decibels::gainToDecibels(value);
        if (decibels < -80) return "-Inf";
        return String{ decibels, 2 } + " dB";
    }
    // case MULTIMODE:          return "Multimode";
    // case LFOSHWAVE:          return "LfoSampleHoldWave";
    // case LFOSINWAVE:         return "LfoSineWave";
    // case LFOSQUAREWAVE:      return "LfoSquareWave";
    // case LFO1AMT:            return "LfoAmount1";
    // case LFO2AMT:            return "LfoAmount2";
    // case LFOFILTER:          return "LfoFilter";
    // case LFOOSC1:            return "LfoOsc1";
    // case LFOOSC2:            return "LfoOsc2";
    case LFOFREQ:            return String{ logsc(value, 0, 50, 120), 2 } + " Hz";
    // case LFOPW1:             return "LfoPw1";
    // case LFOPW2:             return "LfoPw2";
    // case PORTADER:           return "PortamentoDetune";
    // case FILTERDER:          return "FilterDetune";
    // case ENVDER:             return "EnvelopeDetune";
    case PAN1:               
    case PAN2:               
    case PAN3:               
    case PAN4:               
    case PAN5:               
    case PAN6:               
    case PAN7:               
    case PAN8: {
		const auto pan = value - 0.5f;
        if (pan < 0.f) return String{ pan, 2 } + " (Left)";
        if (pan > 0.f) return String{ pan, 2 } + " (Right)";
        return String{ pan, 2 } + " (Center)";
    }
    // case XMOD:               return "Xmod";
    // case OSC2HS:             return "Osc2HardSync";
    case OSC1P:              return String{ (float(value * 4) - 2) * 12.f, 1 } + " Semitones";
    case OSC2P:              return String{ (float(value * 4) - 2) * 12.f, 1 } + " Semitones";
    // case PORTAMENTO:         return "Portamento";
    // case UNISON:             return "Unison";
    // case FLT_KF:             return "FilterKeyFollow";
    // case PW:                 return "PulseWidth";
    // case OSC2Saw:            return "Osc2Saw";
    // case OSC1Saw:            return "Osc1Saw";
    // case OSC1Pul:            return "Osc1Pulse";
    // case OSC2Pul:            return "Osc2Pulse";
    //case VOLUME:             return String{ Decibels::gainToDecibels(linsc(value, 0, 0.30)), 2 };
    // case UDET:               return "VoiceDetune";
    // case OSC2_DET:           return "Oscillator2detune";
    // case CUTOFF:             return "Cutoff";
    // case RESONANCE:          return "Resonance";
    // case ENVELOPE_AMT:       return "FilterEnvAmount";
    // case LATK:               return String{ logsc(value, 4, 60000, 900) / 1000.f, 2};
    // case LDEC:               return String{ logsc(value, 4, 60000, 900) / 1000.f, 2};
    // case LSUS:               return String{ value, 2};
    // case LREL:               return String{ logsc(value, 8, 60000, 900) / 1000.f, 2};
    // case FATK:               return String{ logsc(value, 1, 60000, 900) / 1000.f, 2};
    // case FDEC:               return String{ logsc(value, 1, 60000, 900) / 1000.f, 2};
    // case FSUS:               return String{ value, 2};
    // case FREL:               return String{ logsc(value, 1, 60000, 900) / 1000.f, 2 };

    default:
        break;
    }

    return String{ static_cast<int>(jmap(value, 0.f, 127.f)) };
}

int ObxdAudioProcessor::getParameterIndexFromId (String paramId)
{
    for (size_t i = 0; i < PARAM_COUNT; ++i)
    {
        if (paramId.compare (getEngineParameterId (i)) == 0)
        {
            return int (i);
        }
    }
    
    return -1;
}

void ObxdAudioProcessor::setEngineParameterValue (int index, float newValue, bool notifyToHost)
{
    if (! midiControlledParamSet || index == MIDILEARN || index == UNLEARN)
    {
        lastUsedParameter = index;
    }
    
    programs.currentProgramPtr->values[index] = newValue;
    
    if (notifyToHost){
        apvtState.getParameter(getEngineParameterId(index))->setValueNotifyingHost(newValue);
    } else {
        apvtState.getParameter(getEngineParameterId(index))->setValue(newValue);
    }
    
    //DBG("Set Value Parameter: " << getEngineParameterId(index) << " Val: " << newValue);
    switch (index)
    {
        case SELF_OSC_PUSH:
            synth.processSelfOscPush (newValue);
            break;
        case PW_ENV_BOTH:
            synth.processPwEnvBoth (newValue);
            break;
        case PW_OSC2_OFS:
            synth.processPwOfs (newValue);
            break;
        case ENV_PITCH_BOTH:
            synth.processPitchModBoth (newValue);
            break;
        case FENV_INVERT:
            synth.processInvertFenv (newValue);
            break;
        case LEVEL_DIF:
            synth.processLoudnessDetune (newValue);
            break;
        case PW_ENV:
            synth.processPwEnv (newValue);
            break;
        case LFO_SYNC:
            synth.procLfoSync (newValue);
            break;
        case ECONOMY_MODE:
            synth.procEconomyMode (newValue);
            break;
        case VAMPENV:
            synth.procAmpVelocityAmount (newValue);
            break;
        case VFLTENV:
            synth.procFltVelocityAmount (newValue);
            break;
        case ASPLAYEDALLOCATION:
            synth.procAsPlayedAlloc (newValue);
            break;
        case BENDLFORATE:
            synth.procModWheelFrequency (newValue);
            break;
        case FOURPOLE:
            synth.processFourPole (newValue);
            break;
        case LEGATOMODE:
            synth.processLegatoMode (newValue);
            break;
        case ENVPITCH:
            synth.processEnvelopeToPitch (newValue);
            break;
        case OSCQuantize:
            synth.processPitchQuantization (newValue);
            break;
        case VOICE_COUNT:
            synth.setVoiceCount (newValue);
            break;
        case BANDPASS:
            synth.processBandpassSw (newValue);
            break;
        case FILTER_WARM:
            synth.processOversampling (newValue);
            break;
        case BENDOSC2:
            synth.procPitchWheelOsc2Only (newValue);
            break;
        case BENDRANGE:
            synth.procPitchWheelAmount (newValue);
            break;
        case NOISEMIX:
            synth.processNoiseMix (newValue);
            break;
        case OCTAVE:
            synth.processOctave (newValue);
            break;
        case TUNE:
            synth.processTune (newValue);
            break;
        case BRIGHTNESS:
            synth.processBrightness (newValue);
            break;
        case MULTIMODE:
            synth.processMultimode (newValue);
            break;
        case LFOFREQ:
            synth.processLfoFrequency (newValue);
            break;
        case LFO1AMT:
            synth.processLfoAmt1 (newValue);
            break;
        case LFO2AMT:
            synth.processLfoAmt2 (newValue);
            break;
        case LFOSINWAVE:
            synth.processLfoSine (newValue);
            break;
        case LFOSQUAREWAVE:
            synth.processLfoSquare (newValue);
            break;
        case LFOSHWAVE:
            synth.processLfoSH (newValue);
            break;
        case LFOFILTER:
            synth.processLfoFilter (newValue);
            break;
        case LFOOSC1:
            synth.processLfoOsc1 (newValue);
            break;
        case LFOOSC2:
            synth.processLfoOsc2 (newValue);
            break;
        case LFOPW1:
            synth.processLfoPw1 (newValue);
            break;
        case LFOPW2:
            synth.processLfoPw2 (newValue);
            break;
        case PORTADER:
            synth.processPortamentoDetune (newValue);
            break;
        case FILTERDER:
            synth.processFilterDetune (newValue);
            break;
        case ENVDER:
            synth.processEnvelopeDetune (newValue);
            break;
        case XMOD:
            synth.processOsc2Xmod (newValue);
            break;
        case OSC2HS:
            synth.processOsc2HardSync (newValue);
            break;
        case OSC2P:
            synth.processOsc2Pitch (newValue);
            break;
        case OSC1P:
            synth.processOsc1Pitch (newValue);
            break;
        case PORTAMENTO:
            synth.processPortamento (newValue);
            break;
        case UNISON:
            synth.processUnison (newValue);
            break;
        case FLT_KF:
            synth.processFilterKeyFollow (newValue);
            break;
        case OSC1MIX:
            synth.processOsc1Mix (newValue);
            break;
        case OSC2MIX:
            synth.processOsc2Mix (newValue);
            break;
        case PW:
            synth.processPulseWidth (newValue);
            break;
        case OSC1Saw:
            synth.processOsc1Saw (newValue);
            break;
        case OSC2Saw:
            synth.processOsc2Saw (newValue);
            break;
        case OSC1Pul:
            synth.processOsc1Pulse (newValue);
            break;
        case OSC2Pul:
            synth.processOsc2Pulse (newValue);
            break;
        case VOLUME:
            synth.processVolume (newValue);
            break;
        case UDET:
            synth.processDetune (newValue);
            break;
        case OSC2_DET:
            synth.processOsc2Det (newValue);
            break;
        case CUTOFF:
            synth.processCutoff (newValue);
            break;
        case RESONANCE:
            synth.processResonance (newValue);
            break;
        case ENVELOPE_AMT:
            synth.processFilterEnvelopeAmt (newValue);
            break;
        case LATK:
            synth.processLoudnessEnvelopeAttack (newValue);
            break;
        case LDEC:
            synth.processLoudnessEnvelopeDecay (newValue);
            break;
        case LSUS:
            synth.processLoudnessEnvelopeSustain (newValue);
            break;
        case LREL:
            synth.processLoudnessEnvelopeRelease (newValue);
            break;
        case FATK:
            synth.processFilterEnvelopeAttack (newValue);
            break;
        case FDEC:
            synth.processFilterEnvelopeDecay (newValue);
            break;
        case FSUS:
            synth.processFilterEnvelopeSustain (newValue);
            break;
        case FREL:
            synth.processFilterEnvelopeRelease (newValue);
            break;
        case PAN1:
            synth.processPan (newValue,1);
            break;
        case PAN2:
            synth.processPan (newValue,2);
            break;
        case PAN3:
            synth.processPan (newValue,3);
            break;
        case PAN4:
            synth.processPan (newValue,4);
            break;
        case PAN5:
            synth.processPan (newValue,5);
            break;
        case PAN6:
            synth.processPan (newValue,6);
            break;
        case PAN7:
            synth.processPan (newValue,7);
            break;
        case PAN8:
            synth.processPan (newValue,8);
            break;
    }
    
    //DIRTY HACK
    //This should be checked to avoid stalling on gui update
    //It is needed because some hosts do  wierd stuff
    if (isHostAutomatedChange)
        sendChangeMessage();
}

//==============================================================================
void ObxdAudioProcessor::parameterChanged (const String& parameter, float newValue)
{
    int index = getParameterIndexFromId (parameter);
    
    if ( isPositiveAndBelow (index, PARAM_COUNT) )
    {
        isHostAutomatedChange = false;
        setEngineParameterValue (index, newValue);
        isHostAutomatedChange = true;
    }
}

AudioProcessorValueTreeState& ObxdAudioProcessor::getPluginState()
{
    return apvtState;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new ObxdAudioProcessor();
}



void ObxdAudioProcessor::initMidi(){
    //Documents > Obxd > MIDI > Default.xml
    File default_file = getMidiFolder().getChildFile("Default.xml");
    if (!default_file.exists()){
        bindings.saveFile(default_file);
    }
    
    File midi_config_file = getMidiFolder().getChildFile("Config.xml");
    XmlDocument xmlDoc (midi_config_file);
    std::unique_ptr<XmlElement> ele_file = xmlDoc.getDocumentElementIfTagMatches("File");

    if (ele_file) {
        String file_name = ele_file->getStringAttribute("name");
        // Midi cc loading
        File midi_file = getMidiFolder().getChildFile(file_name);
        if (bindings.loadFile(midi_file)){
            currentMidiPath = midi_file.getFullPathName();
        } else {
            File midi_file = getMidiFolder().getChildFile("Default.xml");
            if (bindings.loadFile(midi_file)){
                currentMidiPath = midi_file.getFullPathName();
            }
        }
    }
}

void ObxdAudioProcessor::updateConfig(){
    File midi_config_file = getMidiFolder().getChildFile("Config.xml");
    XmlDocument xmlDoc (midi_config_file);
    std::unique_ptr<XmlElement> ele_file = xmlDoc.getDocumentElementIfTagMatches("File");
    if (ele_file) {
        File f(currentMidiPath);
        ele_file->setAttribute("name", f.getFileName());
        ele_file->writeTo(midi_config_file.getFullPathName());
    }
}
