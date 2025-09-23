/*
 * OB-Xf - a continuation of the last open source version of OB-Xd.
 *
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more info,
 * see "HISTORY.md" in the root of this repository.
 *
 * This repository is a successor to the last open source release,
 * a version marked as 2.11. Copyright 2013-2025 by the authors
 * as indicated in the original release, and subsequent authors
 * per the GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_CORE_CONSTANTS_H
#define OBXF_SRC_CORE_CONSTANTS_H

#include <cstdint>
#include <juce_core/juce_core.h>

static constexpr int syncedRatesCount{21};
// clang-format off
constexpr std::array<float, syncedRatesCount> syncedRates
{
    1.f/12, // 1/32 T
    1.f/8,  // 1/32
    1.f/6,  // 1/16 T
    3.f/16, // 1/32 D
    1.f/4,  // 1/16
    1.f/3,  // 1/8 T
    3.f/8,  // 1/16 D
    1.f/2,  // 1/8
    2.f/3,  // 1/4 T
    3.f/4,  // 1/8 D
    1,    // 1/4
    4.f/3,  // 1/2 T
    3.f/2,  // 1/4 D
    2.f,    // 1/2
    8.f/3,  // 1/1 T
    3.f,    // 1/2 D
    4.f,    // 1/1
    6.f,    // 1/1 D
    8.f,    // 2/1
    12.f,   // 3/1
    16.f,   // 4/1
};

// This array is backwards. 1/4 (the midpoint) is correct but the
// rate above starts with 1.f / 12 which is a slower rate and this
// starts with the faster rate. We account for this with a silly
// little hack in SynthParam.h
const std::array<std::string, syncedRatesCount> syncedRateNames
{
    "1/32 T",
    "1/32",
    "1/16 T",
    "1/32 D",
    "1/16",
    "1/8 T",
    "1/16 D",
    "1/8",
    "1/4 T",
    "1/8 D",
    "1/4",
    "1/2 T",
    "1/4 D",
    "1/2",
    "1/1 T",
    "1/2 D",
    "1/1",
    "1/1 D",
    "2/1",
    "3/1",
    "4/1",
};
// clang-format on

constexpr int MAX_VOICES = 32;
constexpr int MAX_PROGRAMS = 256;
constexpr int MAX_BEND_RANGE = 48;
constexpr int MAX_PANNINGS = 8;
constexpr uint8_t NUM_PATCHES_PER_GROUP = 16;
constexpr uint8_t NUM_LFOS = 2;
constexpr uint8_t NUM_XPANDER_MODES = 15;
constexpr uint8_t OVERSAMPLE_FACTOR = 2;

constexpr float dc = 1e-18f;
constexpr float ln2 = 0.69314718056f;
constexpr float mult = ln2 / 12.f;
constexpr float twoPi = juce::MathConstants<float>::twoPi;
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float invPi = 1.f / juce::MathConstants<float>::pi;
constexpr float invTwoPi = 1.f / juce::MathConstants<float>::twoPi;
constexpr float halfPi = juce::MathConstants<float>::halfPi;
constexpr float twoByPi = 2.f / juce::MathConstants<float>::pi;

constexpr int fxbVersionNum = 1;

struct fxProgram
{
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize;   // of this chunk, excl. magic + byteSize
    int32_t fxMagic;    // 'FxCk'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numParams;
    char prgName[28];
    float params[1]; // variable no. of parameters
};

struct fxSet
{
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize;   // of this chunk, excl. magic + byteSize
    int32_t fxMagic;    // 'FxBk'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char future[128];
    fxProgram programs[1]; // variable no. of programs
};

struct fxChunkSet
{
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize;   // of this chunk, excl. magic + byteSize
    int32_t fxMagic;    // 'FxCh', 'FPCh', or 'FBCh'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char future[128];
    int32_t chunkSize;
    char chunk[8]; // variable
};

struct fxProgramSet
{
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize;   // of this chunk, excl. magic + byteSize
    int32_t fxMagic;    // 'FxCh', 'FPCh', or 'FBCh'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char name[28];
    int32_t chunkSize;
    char chunk[8]; // variable
};

// Compares a magic value in either endianness.
static inline bool compareMagic(const int32_t magic, const char *name) noexcept
{
    return magic == static_cast<int32_t>(juce::ByteOrder::littleEndianInt(name)) ||
           magic == static_cast<int32_t>(juce::ByteOrder::bigEndianInt(name));
}

static inline int32_t fxbName(const char *name) noexcept
{
    return static_cast<int32_t>(juce::ByteOrder::littleEndianInt(name));
}

static inline int32_t fxbSwap(const int32_t x) noexcept
{
    return static_cast<int32_t>(juce::ByteOrder::swapIfLittleEndian(static_cast<uint32_t>(x)));
}

static inline float fxbSwapFloat(const float x) noexcept
{
#ifdef JUCE_LITTLE_ENDIAN
    union
    {
        uint32_t asInt;
        float asFloat;
    } n{};
    n.asFloat = x;
    n.asInt = juce::ByteOrder::swap(n.asInt);
    return n.asFloat;
#else
    return x;
#endif
}

#endif // CONSTANTS_H