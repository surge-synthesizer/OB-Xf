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
cmake -B Builds/Release -DCMAKE_BUILD_TYPE=Release .
cmake --build Builds/Release --config Release --target obxf-staged
```

This will build supported plugin formats and place them in builds/Release/obxf_products. If you self-build, you are responsible for installing the assets from assets/installer in the appropriate location. When running the plugin or standalone, you will see where OB-Xf attempted to look for assets, if you get it wrong.

If you are on a unix system we provide the following convenience to install the factory assets after a build

```bash
cmake --install Builds/Release
```

You may need a sudo. Like the build, this will use the `CMAKE_INSTALL_PREFIX` for the shared location

## iPad / iOS target (work in progress)

iOS support is being added incrementally. The initial CMake wiring enables iOS-safe formats (`AUv3`, `Standalone`) and disables desktop-only packaging steps.

The current iOS CMake setup targets iPad (`TARGETED_DEVICE_FAMILY=2`).

A starting point for generating an Xcode iOS project is:

```bash
cmake -B Builds/iOS -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DOBXF_IOS_DISABLE_CODESIGN=ON .
cmake --build Builds/iOS --config Release
```

If you want to sign for device install, set your team ID and disable the no-signing default:

```bash
cmake -B Builds/iOS -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DOBXF_IOS_DISABLE_CODESIGN=OFF -DOBXF_IOS_DEVELOPMENT_TEAM=YOURTEAMID .
```

Packaging and asset installation for iOS are not complete yet.
# Copyright

This repository and the source code is under GPL3 license. OB-Xf is and always will be free in all contexts and for all uses, with the source code available and modifiable, and the software usable in any context, free or commercial.
