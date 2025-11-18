Plan of attack for baconpaul chainsaw attack. This updates as I go. Just keeping it in git
so I dont loose it

PARAM chainsaw work

- Rename ParameterAdapter -> something
  
Plan of attack: clean it up some and understand it then run with paramSet on

- pretty sure we don't need needToNotifyHost any more maybe? (this may be patch work)

STUFF I KNOW I BROKE

- Does tghe key command handler do anything?
- MIDI Program Change

STUFF TO ADD

- Open an issue for better undo
  - Blocks when a program changes or an algo runs
  - Redo
  - A UI adjust
- add author field to fxp and make an author tool (even if python)

in parallel

- ED does some basic patch browser design

