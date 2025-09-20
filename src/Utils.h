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

#ifndef OBXF_SRC_UTILS_H
#define OBXF_SRC_UTILS_H

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <fmt/core.h>
#include "filesystem/import.h"

#include "Constants.h"

inline static float getPitch(float index) { return 440.f * expf(mult * index); };

inline static float linsc(float param, const float min, const float max)
{
    return (param) * (max - min) + min;
}

inline static float logsc(float param, const float min, const float max, const float rolloff = 19.f)
{
    return ((expf(param * logf(rolloff + 1.f)) - 1.f) / (rolloff)) * (max - min) + min;
}

static constexpr uint64_t currentStreamingVersion{0x2025'06'24};

inline std::string humanReadableVersion(const uint64_t v)
{
    return fmt::format("{:04x}-{:02x}-{:02x}", (v >> 16) & 0xFFFF, (v >> 8) & 0xFF, v & 0xFF);
}

class Utils final
{
  public:
    Utils();
    ~Utils();

    enum LocationType
    {
        SYSTEM_FACTORY = 0,
        LOCAL_FACTORY = 1,
        USER = 2
    };

    // File System Methods
    [[nodiscard]] juce::File getFactoryFolderInUse() const;
    void resolveFactoryFolderInUse();

    [[nodiscard]] juce::File getSystemFactoryFolder() const;
    [[nodiscard]] juce::File getLocalFactoryFolder() const;
    [[nodiscard]] juce::File getDocumentFolder() const;
    void createDocumentFolderIfMissing();

    // Theme Management
    struct ThemeLocation
    {
        LocationType locationType;
        juce::String dirName;
        juce::File file;

        bool operator==(const ThemeLocation &other) const
        {
            return locationType == other.locationType && dirName == other.dirName;
        }
    };
    [[nodiscard]] std::vector<juce::File> getThemeFolders() const;
    [[nodiscard]] juce::File getThemeFolderFor(LocationType loc) const;
    [[nodiscard]] const std::vector<ThemeLocation> &getThemeLocations() const;
    [[nodiscard]] ThemeLocation getCurrentThemeLocation() const;
    void setCurrentThemeLocation(const ThemeLocation &loc);
    void scanAndUpdateThemes();

    // Bank Management
    struct BankLocation
    {
        LocationType locationType;
        juce::String dirName;
        juce::File file;

        bool operator==(const BankLocation &other) const
        {
            return locationType == other.locationType && dirName == other.dirName;
        }
    };
    [[nodiscard]] std::vector<juce::File> getBanksFolders() const;
    [[nodiscard]] juce::File getBanksFolderFor(LocationType loc) const;
    [[nodiscard]] const std::vector<BankLocation> &getBankLocations() const;
    [[nodiscard]] BankLocation getCurrentBankLocation() const;
    bool loadFromFXBFile(const juce::File &fxbFile);
    bool loadFromFXBLocation(const BankLocation &fxbFile);

    // Midi Management
    struct MidiLocation
    {
        LocationType locationType;
        juce::String dirName;
        juce::File file;

        bool operator==(const MidiLocation &other) const
        {
            return locationType == other.locationType && dirName == other.dirName;
        }
    };

    [[nodiscard]] juce::File getMidiFolderFor(LocationType loc) const;
    [[nodiscard]] std::vector<juce::File> getMidiFolders() const;
    [[nodiscard]] const std::vector<MidiLocation> &getMidiLocations() const;
    [[nodiscard]] MidiLocation getCurrentMidiLocation() const;
    void setCurrentMidiLocation(const MidiLocation &loc);

    // GUI Settings
    void setGuiSize(int size);

    [[nodiscard]] int getGuiSize() const { return gui_size; }
    [[nodiscard]] float getPixelScaleFactor() const { return physicalPixelScaleFactor; }
    void setPixelScaleFactor(const float factor) { physicalPixelScaleFactor = factor; }

    // FXB

    void scanAndUpdateBanks();

    // Zoom
    void setDefaultZoomFactor(float f);
    float getDefaultZoomFactor() const;

    // banks
    bool deleteBank();
    void saveBank() const;
    bool saveBank(const juce::File &fxbFile);

    [[nodiscard]] bool saveFXBFile(const juce::File &fxbFile) const;

    [[nodiscard]] juce::String getCurrentProgram() const { return currentPatch; }

    bool saveFXPFile(const juce::File &fxpFile) const;

    bool loadPatch(const juce::File &fxpFile);

    bool savePatch(const juce::File &fxpFile);

    void savePatch();

    void serializePatch(juce::MemoryBlock &memoryBlock) const;

    void changePatchName(const juce::String &name) const;

    void newPatch(const juce::String &name) const;

    void initializePatch() const;

    bool loadFromFXPFile(const juce::File &fxpFile);

    juce::File getPresetsFolder() const { return getDocumentFolder().getChildFile("Patches"); }

    bool isMemoryBlockAPatch(const juce::MemoryBlock &mb);

    // should refactor all callbacks to be like this? or not? :-)
    using HostUpdateCallback = std::function<void()>;
    void setHostUpdateCallback(HostUpdateCallback callback)
    {
        hostUpdateCallback = std::move(callback);
    }

    // callbacks
    std::function<bool(juce::MemoryBlock &)> loadMemoryBlockCallback;
    std::function<void(juce::MemoryBlock &)> getStateInformationCallback;
    std::function<int()> getNumProgramsCallback;
    std::function<void(juce::MemoryBlock &)> getCurrentProgramStateInformation;
    std::function<int()> getNumPrograms;
    std::function<void(char *, int)> copyProgramNameToBuffer;
    std::function<void(const juce::String &)> setPatchName;
    std::function<void()> resetPatchToDefault;
    std::function<void()> sendChangeMessage;
    std::function<void(int)> setCurrentProgram;
    std::function<bool(int, const juce::String &)> isProgramNameCallback;

    juce::File fsPathToJuceFile(const fs::path &) const;
    fs::path juceFileToFsPath(const juce::File &) const;

    void setPluginAPIScale(float s) { pluginApiScale = s; }
    float getPluginAPIScale() const { return pluginApiScale; }

  private:
    float pluginApiScale{1.f};
    // Config Management
    std::unique_ptr<juce::PropertiesFile> config;
    juce::InterProcessLock configLock;

    LocationType resolvedFactoryLocationType{SYSTEM_FACTORY};

    void updateConfig();

    // File Collections
    std::vector<ThemeLocation> themeLocations;
    std::vector<BankLocation> bankLocations;
    std::vector<MidiLocation> midiLocations;

    // Current States
    ThemeLocation currentTheme;
    BankLocation currentBank;
    MidiLocation currentMidi;

    int gui_size{};
    float physicalPixelScaleFactor{};

    HostUpdateCallback hostUpdateCallback;

    // patch
    juce::String currentPatch;
    juce::File currentPatchFile;
};

#endif // OBXF_SRC_UTILS_H
