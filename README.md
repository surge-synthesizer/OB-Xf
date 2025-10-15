# About

OB-Xf is a continuation of the last open source version of OB-Xd by [2DaT](https://github.com/2DaT/Obxd) and later
[discoDSP](https://github.com/reales/OB-Xd), bringing together several efforts going on in the audio space and
combining them inside the Surge Synth Team infrastructure.

The synth is currently in a beta phase, with a few features still under development, but the plugin is feature stable and working rather well, we believe.

# Installation

We provide installers (signed DMG for macOS, EXE for Windows x64/ARM64/ARM64EC, DEB for Linux x86_64 built on Ubuntu 20) [here](https://github.com/surge-synthesizer/OB-Xf/releases/tag/Nightly). See that page for installation instructions.

# Compatibility with OB-Xd

Patches and banks created by OB-Xd are **NOT (!)** compatible with OB-Xf, due to a number of changes we decided to do for certain parameters.
Patch conversion is likely possible for the most part, but is not the immediate priority, and 100% fidelity to OB-Xd cannot be guaranteed.

# Building

Using CMake:

```bash
git submodule update --init --recursive
cmake -B Builds/Release .
cmake --build Builds/Release --config Release --target obxf-staged
```

This will build supported plugin formats and place them in builds/Release/obxf_products. If you self-build, you are responsible for installing the assets from assets/installer in the appropriate location. When running the plugin or standalone, you will see where OB-Xf attempted to look for assets, if you get it wrong.

# Copyright

This repository and the source code is under GPL3 license. OB-Xf is and always will be free in all contexts and for all uses, with the source code available and modifiable, and the software usable in any context, free or commercial.
