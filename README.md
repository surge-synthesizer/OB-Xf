# About

OB-Xf is a continuation of the last open source version of [OB-Xd by reales](https://github.com/reales/OB-Xd), bringing
together several efforts going on in the audio space and combining them inside the surge synth team infrastructure.

Source code can be compiled with [JUCE 8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) using CMake.

# Changelog
- This project now has modern flexible resizing
- all data races and memory issues in the original repo
have been fixed(afaik). 
- This now passes pluginval on strictness 10 for both AU & VST3 [LV2 not tested yet], with 
ThreadSanitizer & AddressSanitizer enabled.
- I have also added a midi keyboard component within the UI. To include this you would need to add `<VALUE NAME="midiKeyboard" x="150" y="490" w="850" h="80"/>`
to your themes `coords.xml` file.

# Themes
I will be adding a new theme soon, but for now you can use the original themes or other custom themes people have made over the years. If you want to add
the keyboard to your theme, you will need to add the above line to your `coords.xml` file and update your background image to account for the space.

Themes are expected to be stored in ~/Documents like the original repo. I'm not sure this will remain the case in the future, but for backwards compatibility for now it remains
the same as the original.

# Building

Using CMake.

```bash
cmake -B Builds/Release .
cmake --build Builds/Release --config Release
```

VST3, AU, STANDALONE, LV2 and CLAP are supported.

# Copyright

The original author [2Dat](https://github.com/2DaT) granted discoDSP a OB-Xd private use license on December 2022.

This repository and the source code is under GPL3 license.

Files from Source > Images folder in this commit have Attribution-ShareAlike 2.0 Generic (CC BY-SA 2.0) license.

Other images and applicable resources have all rights reserved and copyrighted to discoDSP (no copying, modifying, merging, selling, leasing, redistributing, assigning, or transferring, remove, alter, deface, overprint or otherwise obscure licensor patent, trademark, service mark or copyright, publish or distribute in any form of electronic or printed communication those materials without permission).

# Continuous Integration

CI runs on every PR and on every merge to main.

On every pr, it builds the target `obxf-staged` which builds all assets for the installer and places them into a single directory.

For CI, it builds `obxf-installer` which builds `obxf-staged` and then creates a zip file on linux and windows and a signed mac installer on macOS.

These nightly assets are incomplete as of this writing, without the themes or resources needed to run the synth.

#
[<img src="https://github.com/user-attachments/assets/6461b43e-1e53-4edf-b8f6-9256e7031187" alt="develop-logo-pluginval" width="80"/>](https://www.tracktion.com/develop/pluginval)
