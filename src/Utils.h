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

static constexpr uint64_t currentStreamingVersion{0x2025'11'14};

inline std::string humanReadableVersion(const uint64_t v)
{
    return fmt::format("{:04x}-{:02x}-{:02x}", (v >> 16) & 0xFFFF, (v >> 8) & 0xFF, v & 0xFF);
}

inline uint64_t fromHumanReadableVersion(const std::string &v)
{
    if (v.size() != 10)
        return 0;
    if (v[4] != '-' && v[7] != '-')
        return 0;
    return std::stoul(v.substr(0, 4), nullptr, 16) << 16 |
           std::stoul(v.substr(5, 2), nullptr, 16) << 8 | std::stoul(v.substr(8, 2), nullptr, 16);
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
        USER = 2,
        EMBEDDED = 3
    };

    static std::string toString(LocationType lt)
    {
        switch (lt)
        {
        case SYSTEM_FACTORY:
            return "System Factory";
        case LOCAL_FACTORY:
            return "Local Factory";
        case USER:
            return "User";
        case EMBEDDED:
            return "Embedded";
        }
        return "ERR";
    }

#if JUCE_WINDOWS
    const juce::File embeddedThemeSentinel{"C:\\embedded-theme\\Default Vector"};
#else
    const juce::File embeddedThemeSentinel{"/embedded-theme/Default Vector"};
#endif

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

    // Bank Management
    struct PatchTreeNode
    {
        LocationType locationType;
        bool isFolder;
        juce::String displayName;
        juce::File file;
        std::vector<PatchTreeNode> children;

        bool operator==(const PatchTreeNode &other) const
        {
            return locationType == other.locationType && displayName == other.displayName &&
                   isFolder == other.isFolder;
        }
        void print(std::string pfx = "")
        {
            if (!obxf_log::patches)
                return;
            OBLOG(patches, pfx << " " << displayName << " (" << toString(locationType) << ")");
            for (auto &child : children)
                child.print(pfx + "--");
        }
    } patchRoot;
    [[nodiscard]] juce::File getPatchFolderFor(LocationType loc) const;
    [[nodiscard]] const PatchTreeNode &getPatchRoot() const;
    void rescanPatchTree();
    void scanPatchFolderInto(PatchTreeNode &parent, LocationType lt, juce::File &folder);

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

    // Patch Management
    void scanAndUpdatePatchList();

    // FXB

    void scanAndUpdateBanks();

    // Zoom
    void setDefaultZoomFactor(float f);
    float getDefaultZoomFactor() const;

    // Software Renderer (windows only)
    void setUseSoftwareRenderer(bool b);
    bool getUseSoftwareRenderer() const;

    // banks
    void saveBank() const;
    bool saveBank(const juce::File &fxbFile);

    [[nodiscard]] bool saveFXBFile(const juce::File &fxbFile) const;

    [[nodiscard]] juce::String getCurrentProgram() const { return currentPatch; }

    bool saveFXPFile(const juce::File &fxpFile) const;
    bool saveFXPFileFrom(const juce::File &fxpFile, const int index = -1) const;

    bool loadPatch(const juce::File &fxpFile);

    bool savePatch(const juce::File &fxpFile);
    bool savePatchFrom(const juce::File &fxpFile, const int index = -1);

    void savePatch();

    std::function<void(juce::MemoryBlock &, const int)> pastePatchCallback;

    void copyPatch(const int index);
    void pastePatch(const int index);

    bool isPatchInClipboard();

    juce::MemoryBlock serializePatch(juce::MemoryBlock &memoryBlock, const int index = -1) const;

    void changePatchName(const juce::String &name) const;

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

    void loadFactoryBank();

    // callbacks
    std::function<bool(juce::MemoryBlock &, const int)> loadMemoryBlockCallback;
    std::function<void(juce::MemoryBlock &)> getStateInformationCallback;
    std::function<int()> getNumProgramsCallback;
    std::function<void(const int, juce::MemoryBlock &)> getProgramStateInformation;
    std::function<int()> getNumPrograms;
    std::function<void(const int, char *, int)> copyProgramNameToBuffer;
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
