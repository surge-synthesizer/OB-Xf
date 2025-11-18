Plan of attack for baconpaul chainsaw attack. This updates as I go. Just keeping it in git
so I dont loose it

PARAM chainsaw work

- Rename ParameterAdapter -> something

  
Plan of attack: clean it up some and understand it then run with paramSet on

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

