# About

OB-Xf is a continuation of the last open source version of OB-Xd by [2DaT](https://github.com/2DaT/Obxd) and later
[discoDSP](https://github.com/reales/OB-Xd), bringing together several efforts going on in the audio space and
combining them inside the Surge Synth Team infrastructure.

The synth is currently in a beta phase, with a few features still under development, but the
plugins and features stable and working, we beleive.

# Installing the synth

We provide installers (signed dmg for macOS, exe for windows x64 and arm/armec, 
deb for linux x86_64 built on Ubuntu20) [here](https://github.com/surge-synthesizer/OB-Xf/releases/tag/Nightly).
See that page for installation instructions and so forth.

# Compatibility with OB-Xd

Patches and banks created by OB-Xd are **NOT (!)** compatible with OB-Xf, due to a number of changes we decided to do to some of the parameters.
Patch conversion is likely possible for the most part, but is not the immediate priority, and 100% fidelity to OB-Xd cannot be guaranteed.

# Building

Using CMake:

```bash
cmake -B Builds/Release .
cmake --build Builds/Release --config Release --target obxf-staged
```

This will build supported plugin formats and place them in Builds/Release/obxf_products. If you self build
you are responsible for installing the assets from assets/install in the appropriate location also.

# Copyright

The original author [Vadim Filatov (2DaT)](https://github.com/2DaT) granted discoDSP an OB-Xd private use license on December 2022.

This repository and the source code is under GPL3 license.

Other images and applicable resources have all rights reserved and copyrighted to discoDSP (no copying, modifying, merging, selling, leasing, redistributing, assigning, or transferring, remove, alter, deface, overprint or otherwise obscure licensor patent, trademark, service mark or copyright, publish or distribute in any form of electronic or printed communication those materials without permission).
