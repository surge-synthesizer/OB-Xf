Plan of attack for baconpaul chainsaw attack. This updates as I go. Just keeping it in git
so I dont loose it

- All calls to current program in editor should die
- All calls to current program in processor should die

- Utils and the callbacks
  - What does each callback do and do we need it? 
  - Utils has lots of ways to state manager are they all used?
  - Big tagged redundant set in Utils.h around REWORK comment
  
- Program Change probably does nothing. What's the onProgramChange callback now?
- processActiveProgramChanged and below review
- Who calls sendChangeMessage and why
- Can we remove setupPatchNumberMenu now?
- make flat list of factory and show that as plugin edge API. test in Reap and Log
- remove all calls to setCurrentProgram from inside the code - basically make sure those are edge apis except
  on factory patch load
- add author field to fxp

then the next set of chainsaw work

- juce params written more coherently since set current program gone so really just on load or change
- consolidate param manager, param adapter, etc...
- Fix the udpate queues onto the engine timing thing on patch load. Probably just need to lock updates on load

- Does tghe key command handler do anything?

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