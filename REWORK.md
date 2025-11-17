Plan of attack for baconpaul chainsaw attack. This updates as I go. Just keeping it in git
so I dont loose it

PARAM chainsaw work

- Whats the current rela relationship between (+ means done - means todo)
  - IProgramState
    - single interface implementing updateProgramValue virtual
    - Implemented by Processor
  - IParameterState
    - Interace with getMidiControlledParamSet, get/set Last Used, and getIsHostAutiomated
    - Implementef by Processor
  - ParameterAdapter.h
    - Includes ParameterManagerAdapter 
    - hasa ParameterManager
  - ParameterInfo
  - ParameterAttachment
  + ParameterManager.h
    - is a listener to params
    
  
Plan of attack: clean it up some and understand it then run with paramSet on

- review applyActiveProgramValuesToJUCEParameters
- processActiveProgramChanged and below review
- Who calls sendChangeMessage and why
- updateProgramValue is an override yet is getting called a lot. From ParameterManagerAdapter.
- juce params written more coherently since set current program gone so really just on load or change
- consolidate param manager, param adapter, etc...
- Fix the udpate queues onto the engine timing thing on patch load. Probably just need to lock updates on load
- pretty sure we don't need needToNotifyHost any more maybe? (this may be patch work)

STUFF I KNOW I BROKE

- Selection in the patch menu and patch number menu. (And do we even need patch number menu?)
- Does tghe key command handler do anything?
- MIDI Program Change

STUFF TO ADD

- Proper undo
- add author field to fxp and make an author tool (even if python)



in parallel

- ED does some basic patch browser design


Classes to make sure we need and need all their methods

```
src/Utils.h:class Utils final
src/interface/IProgramState.h:class IProgramState
src/interface/IParameterState.h:class IParameterState : virtual public juce::ChangeBroadcaster
src/PluginEditor.h:class ObxfAudioProcessorEditor final : public juce::AudioProcessorEditor,
src/midi/MidiHandler.h:class MidiHandler
src/PluginProcessor.h:class ObxfAudioProcessor final : public juce::AudioProcessor,
src/state/StateManager.h:class StateManager final : public juce::ChangeBroadcaster
src/utilities/KeyCommandHandler.h:class KeyCommandHandler final : public juce::ApplicationCommandTarget
src/parameter/FIFO.h:template <size_t Capacity> class FIFO
src/parameter/ParameterManager.h:class ParameterManager : public juce::AudioProcessorParameter::Listener
src/parameter/ParameterAdapter.h:class ParameterManagerAdapter
src/parameter/ValueAttachment.h:template <typename T> class ValueAttachment final : juce::Value::Listener
src/parameter/ParameterAttachment.h:class Attachment
src/Utils.h:    struct ThemeLocation
src/Utils.h:    struct BankLocation
src/Utils.h:    struct MidiLocation
src/core/Constants.h:struct fxProgram
src/core/Constants.h:struct fxSet
src/core/Constants.h:struct fxChunkSet
src/core/Constants.h:struct fxProgramSet
src/midi/MidiHandler.cpp:struct MidiHandler::LagHandler
src/midi/MidiHandler.h:    struct LagHandler;
src/midi/MidiHandler.h:    friend struct LagHandler;
src/PluginProcessor.h:    struct ObxfParams
src/PluginProcessor.h:        struct iterator
src/PluginProcessor.h:    struct SharedUIState
src/state/StateManager.h:    struct DAWExtraState
src/utilities/KeyCommandHandler.h:struct KeyPressCommandIDs
src/utilities/KeyCommandHandler.h:struct MenuAction
src/parameter/FIFO.h:struct ParameterChange
src/parameter/SynthParam.h:struct ObxfParameterFloat : juce::AudioParameterFloat
src/parameter/ParameterInfo.h:struct ParameterInfo
```