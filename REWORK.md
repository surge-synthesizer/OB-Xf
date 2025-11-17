Plan of attack for baconpaul chainsaw attack. This updates as I go. Just keeping it in git
so I dont loose it

PARAM chainsaw work

+ ParameterManager -> ParameterUpdateHandler

- ParameterAdapter -> something

- Whats the current rela relationship between (+ means done - means todo)
  - IProgramState
    - single interface implementing updateProgramValue virtual
    - Implemented by Processor
  - IParameterState
    - What is 'getIsHostAutomatedChange' on IParameterState
    - Implementef by Processor
    - Implements sentChangeMessage (but why?)
  - ParameterAdapter.h
    - Includes ParameterManagerAdapter 
    - hasa ParameterManager
    - setEngineparameterValue:
      - How is 'notifySet'
  + ParameterInfo
  + ParameterAttachment
    + Hooks a UI element up to a parameter
    + Document and Cleanup
  + ParameterManager.h
    + is a listener to params
    
  
Plan of attack: clean it up some and understand it then run with paramSet on

+review applyActiveProgramValuesToJUCEParameters
+ processActiveProgramChanged and below review
+ Who calls sendChangeMessage and why
+ updateProgramValue is an override yet is getting called a lot. From ParameterManagerAdapter.
  (answer - its an internal one)
- juce params written more coherently since set current program gone so really just on load or change
- Fix the udpate queues onto the engine timing thing on patch load. Probably just need to lock updates on load
- pretty sure we don't need needToNotifyHost any more maybe? (this may be patch work)

STUFF I KNOW I BROKE

- Selection in the patch menu and patch number menu. (And do we even need patch number menu?)
- Does tghe key command handler do anything?
- MIDI Program Change
- Prev/Next deals badly around 0/init

STUFF TO ADD

- Proper undo
- add author field to fxp and make an author tool (even if python)

in parallel

- ED does some basic patch browser design

