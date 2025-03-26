#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <juce_core/juce_core.h>

constexpr int fxbVersionNum = 1;

struct fxProgram {
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize; // of this chunk, excl. magic + byteSize
    int32_t fxMagic; // 'FxCk'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numParams;
    char prgName[28];
    float params[1]; // variable no. of parameters
};

struct fxSet {
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize; // of this chunk, excl. magic + byteSize
    int32_t fxMagic; // 'FxBk'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char future[128];
    fxProgram programs[1]; // variable no. of programs
};

struct fxChunkSet {
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize; // of this chunk, excl. magic + byteSize
    int32_t fxMagic; // 'FxCh', 'FPCh', or 'FBCh'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char future[128];
    int32_t chunkSize;
    char chunk[8]; // variable
};

struct fxProgramSet {
    int32_t chunkMagic; // 'CcnK'
    int32_t byteSize; // of this chunk, excl. magic + byteSize
    int32_t fxMagic; // 'FxCh', 'FPCh', or 'FBCh'
    int32_t version;
    int32_t fxID; // fx unique id
    int32_t fxVersion;
    int32_t numPrograms;
    char name[28];
    int32_t chunkSize;
    char chunk[8]; // variable
};

// Compares a magic value in either endianness.
static inline bool compareMagic(int32_t magic, const char *name) noexcept {
    return magic == (int32_t) juce::ByteOrder::littleEndianInt(name)
           || magic == (int32_t) juce::ByteOrder::bigEndianInt(name);
}

static inline int32_t fxbName(const char *name) noexcept { return (int32_t) juce::ByteOrder::littleEndianInt(name); }

static inline int32_t fxbSwap(const int32_t x) noexcept {
    return (int32_t) juce::ByteOrder::swapIfLittleEndian((uint32_t) x);
}

static inline float fxbSwapFloat(const float x) noexcept {
#ifdef JUCE_LITTLE_ENDIAN
    union {
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

enum class Tooltip {
    Disable = 0,
    StandardDisplay,
    FullDisplay
};

struct KeyPressCommandIDs {
    static constexpr int buttonNextProgram = 1;
    static constexpr int buttonPrevProgram = 2;
    static constexpr int buttonPadNextProgram = 3;
    static constexpr int buttonPadPrevProgram = 4;
};

struct MenuAction {
    static constexpr int Cancel = 0;
    static constexpr int ToggleMidiKeyboard = 1;
    static constexpr int ImportPreset = 2;
    static constexpr int ImportBank = 3;
    static constexpr int ExportBank = 4;
    static constexpr int ExportPreset = 5;
    static constexpr int SavePreset = 6;
    static constexpr int NewPreset = 7;
    static constexpr int RenamePreset = 8;
    static constexpr int DeletePreset = 9;
    static constexpr int DeleteBank = 10;
    static constexpr int ShowBanks = 11;
    static constexpr int CopyPreset = 12;
    static constexpr int PastePreset = 13;
    static constexpr int LoadBank = 14;
};


#endif // CONSTANTS_H
