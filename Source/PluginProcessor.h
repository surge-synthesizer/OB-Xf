/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright © 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/SynthEngine.h"
//#include <stack>
#include "Engine/midiMap.h"
#include "Engine/ObxdBank.h"

//==============================================================================
const int fxbVersionNum = 1;
using namespace juce;
struct fxProgram
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCk'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numParams;
	char prgName[28];
	float params[1];        // variable no. of parameters
};

struct fxSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxBk'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char future[128];
	fxProgram programs[1];  // variable no. of programs
};

struct fxChunkSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char future[128];
	int32 chunkSize;
	char chunk[8];          // variable
};

struct fxProgramSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char name[28];
	int32 chunkSize;
	char chunk[8];          // variable
};

// Compares a magic value in either endianness.
static inline bool compareMagic (int32 magic, const char* name) noexcept
{
	return magic == (int32) ByteOrder::littleEndianInt (name)
	|| magic == (int32) ByteOrder::bigEndianInt (name);
}

static inline int32 fxbName (const char* name) noexcept   { return (int32) ByteOrder::littleEndianInt (name); }
static inline int32 fxbSwap (const int32 x) noexcept   { return (int32) ByteOrder::swapIfLittleEndian ((uint32) x); }

static inline float fxbSwapFloat (const float x) noexcept
{
#ifdef JUCE_LITTLE_ENDIAN
	union { uint32 asInt; float asFloat; } n;
	n.asFloat = x;
	n.asInt = ByteOrder::swap (n.asInt);
	return n.asFloat;
#else
	return x;
#endif
}

enum class Tooltip
{
	Disable = 0,
	StandardDisplay,
	FullDisplay
};
//==============================================================================
/**
*/
class ObxdAudioProcessor  : public AudioProcessor,
	                        public AudioProcessorValueTreeState::Listener,
	                        public ChangeBroadcaster
{
public:
    //==============================================================================
    ObxdAudioProcessor();
    ~ObxdAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
	
	//==============================================================================
	void processMidiPerSample(MidiBufferIterator* iter, const MidiBuffer& midiBuffer, const int samplePos);
	bool getNextEvent(MidiBufferIterator* iter, const MidiBuffer& midiBuffer, const int samplePos);

	//==============================================================================
    void initAllParams();
    void initMidi();
    const String getInputChannelName (int channelIndex) const override;  // WATCH OUT!
    const String getOutputChannelName (int channelIndex) const override;  // WATCH OUT!
    bool isInputChannelStereoPair (int index) const override;  // WATCH OUT!
    bool isOutputChannelStereoPair (int index) const override;  // WATCH OUT!

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;
	const String getName() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    void setCurrentProgram (int index, bool updateHost);
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
	void setCurrentProgramStateInformation (const void* data,int sizeInBytes) override;
	void getCurrentProgramStateInformation (MemoryBlock& destData) override;

	//==============================================================================
	void scanAndUpdateBanks();
    void scanAndUpdateSkins();
	const Array<File>& getBankFiles() const;
    const Array<File>& getSkinFiles() const;
    bool deleteBank();
    bool loadPreset(const File& fxpFile);
    bool savePreset(const File& fxpFile);
	void serializePreset(MemoryBlock& memoryBlock);		// Copy the current Preset into a MemoryBlock
    void changePresetName(const String &name);
    void newPreset(const String &name);
    void deletePreset();
    
	bool loadFromFXPFile(const File& fxbFile);
	bool loadFromFXBFile(const File& fxbFile);
	bool isMemoryBlockAPreset(const MemoryBlock& memoryBlock);
	bool loadFromMemoryBlock(MemoryBlock& memoryBlock);
    bool saveFXBFile(const File& fxbFile);
    bool saveFXPFile(const File& fxpFile);
    bool saveBank(const File& fxbFile);
	bool restoreProgramSettings(const fxProgram* const prog);
	File getCurrentBankFile() const;

    MidiMap &getMidiMap(){ return bindings; }
	//==============================================================================
	const ObxdBank& getPrograms() const { return programs; }

	//==============================================================================
	File getDocumentFolder() const;
	File getSkinFolder() const;
    File getPresetsFolder() const;
	File getBanksFolder() const;
    File getMidiFolder() const;
    
	File getCurrentSkinFolder() const;
	void setCurrentSkinFolder(const String& folderName);
    void setGuiSize(const int gui_size);
	Tooltip getTooltipBehavior() const;
	void setTooltipBehavior(const Tooltip tooltip);
    //==============================================================================
    static String getEngineParameterId (size_t);
	static String getTrueParameterValueFromNormalizedRange(size_t, float normalizedValue);

    int getParameterIndexFromId (String);
    void setEngineParameterValue (int, float, bool notifyToHost= false);
    void parameterChanged (const String&, float) override;
    AudioProcessorValueTreeState& getPluginState();
    
    bool getShowPresetBar(){
        return this->showPresetBar;
    }
    
    void setShowPresetBar(bool val){
        this->showPresetBar = val;
        config->setValue("presetnavigation", this->showPresetBar);
    }
private:
	//==============================================================================
	bool isHostAutomatedChange;

	int lastMovedController;
	int lastUsedParameter;

	MidiMessage* nextMidi{};
	MidiMessage* midiMsg{};
	
	bool midiControlledParamSet;

	bool hasMidiMessage{};
	int midiEventPos{};

	SynthEngine synth;
	ObxdBank programs;

	
public:
    float physicalPixelScaleFactor{};
    int gui_size;
    String currentSkin;
	String currentBank;
    File currentBankFile;
    void saveBank();
    
    String currentMidiPath;
    String currentPreset;
    File currentPresetFile;
    void savePreset();
    MidiMap bindings;
    bool showPresetBar = false;
    
    void updateConfig();
private:
	Array<File> bankFiles;
    Array<File> skinFiles;
	Tooltip tooltipBehavior;

    std::unique_ptr<PropertiesFile> config;
	InterProcessLock configLock;
    
    //==============================================================================
    AudioProcessorValueTreeState apvtState;
    UndoManager                  undoManager;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObxdAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
