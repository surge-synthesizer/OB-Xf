#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <juce_core/juce_core.h>

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

enum class Tooltip
{
    Disable = 0,
    StandardDisplay,
    FullDisplay
};

#endif // CONSTANTS_H
