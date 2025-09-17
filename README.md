# About

OB-Xf is a continuation of the last open source version of OB-Xd by [2DaT](https://github.com/2DaT/Obxd) and later
[discoDSP](https://github.com/reales/OB-Xd), bringing together several efforts going on in the audio space and
combining them inside the Surge Synth Team infrastructure.

This is a work in progress and still in alpha stage.

Source code can be compiled with [JUCE 8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) using CMake.

# Changelog

- Modern flexible resizing.
- All data races and memory issues have been fixed, to the best of our knowledge.
- Passes pluginval on strictness 10 for both AU & VST3 [LV2 not tested yet], with ThreadSanitizer & AddressSanitizer enabled.

# Assets

For now, you can use a cool new theme created by @satYatunes and arturrembe, which is available in the `assets` folder of this repository.

To use the theme, follow these steps:

1. **Copy the entire `Surge Synth Team` folder:**
    - From: `assets/Surge Synth Team`
    - To:
        - **macOS:**  
          `~/Documents/Surge Synth Team`
        - **Windows:**  
          `C:\Users\<YourUserName>\Documents\Surge Synth Team`
        - **Linux:**  
          `~/Documents/Surge Synth Team` (or similar)

   For example, on macOS or Linux:
    ```sh
    cp -R assets/Surge\ Synth\ Team ~/Documents/
    ```
   On Windows, copy the `Surge Synth Team` folder from `assets` to your `Documents` folder.

This setup is required for the synth to properly show its interface.

# Compatibility with OB-Xd

Patches and banks created by OB-Xd are **NOT (!)** compatible with OB-Xf, due to a number of changes we decided to do to some of the parameters.
Patch conversion is likely possible for the most part, but is not the immediate priority, and 100% fidelity to OB-Xd cannot be guaranteed.

# Building

Using CMake:

```bash
cmake -B Builds/Release .
cmake --build Builds/Release --config Release
```

VST3, AU, STANDALONE, LV2 and CLAP are supported.

# Copyright

The original author [Vadim Filatov (2DaT)](https://github.com/2DaT) granted discoDSP an OB-Xd private use license on December 2022.

This repository and the source code is under GPL3 license.

Other images and applicable resources have all rights reserved and copyrighted to discoDSP (no copying, modifying, merging, selling, leasing, redistributing, assigning, or transferring, remove, alter, deface, overprint or otherwise obscure licensor patent, trademark, service mark or copyright, publish or distribute in any form of electronic or printed communication those materials without permission).

# Continuous Integration

CI runs on every pull request and on every merge to ˙main˙ branch.

On every pull request, it builds the target `obxf-staged`, which builds all assets for the installer and places them into a single directory.

For CI, it builds `obxf-installer` which builds `obxf-staged` and then creates a ZIP file for Linux and Windows and a signed installer for macOS.

These nightly assets are incomplete as of this writing, without the themes or resources needed to run the synth.
