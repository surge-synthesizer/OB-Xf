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
            return "Factory";
        case LOCAL_FACTORY:
            return "Factory";
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

    // Patch Management
    struct PatchTreeNode
    {
        using ptr_t = std::shared_ptr<PatchTreeNode>;
        using weakptr_t = std::weak_ptr<PatchTreeNode>;
        LocationType locationType;
        bool isFolder;
        juce::String displayName;
        juce::File file;
        int index{-1};
        int indexInParent{-1};
        std::pair<int, int> childRange{-1, -1};
        std::vector<int> nonFolderChildIndices;
        weakptr_t parent{};

        ~PatchTreeNode()
        {
            // Handy to debug pointer leaks
            // OBLOG(general, "Destroying " << displayName);
        }
        bool operator==(const PatchTreeNode &other) const
        {
            return locationType == other.locationType && displayName == other.displayName &&
                   isFolder == other.isFolder;
        }

        std::vector<PatchTreeNode::ptr_t> children;

        void print(std::string pfx = "")
        {
            if (!obxf_log::patches)
                return;
            if (isFolder)
            {
                OBLOG(patches, pfx << " FOLDER> [" << displayName << "] (" << toString(locationType)
                                   << ")" << " childIndexRange=" << childRange.first << " to "
                                   << childRange.second);
                ;
            }
            else
            {
                OBLOG(patches, pfx << " PATCH> [" << displayName << "] (" << toString(locationType)
                                   << ")" << " index=" << index
                                   << " indexInParent=" << indexInParent);
            }
            for (auto &child : children)
                child->print(pfx + "--");
        }
    };

    PatchTreeNode::ptr_t patchRoot;

    std::vector<PatchTreeNode::ptr_t> patchesAsLinearList;
    int32_t lastFactoryPatch{0};

    [[nodiscard]] juce::File getPatchFolderFor(LocationType loc) const;
    [[nodiscard]] juce::File getUserPatchFolder() const
    {
        return getPatchFolderFor(LocationType::USER);
    }
    [[nodiscard]] const PatchTreeNode &getPatchRoot() const;
    void rescanPatchTree();
    void scanPatchFolderInto(const PatchTreeNode::ptr_t &parent, LocationType lt,
                             juce::File &folder);

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
    [[nodiscard]] const std::vector<MidiLocation> &getMidiLocations() const
    {
        return midiLocations;
    };

    // GUI Settings
    void setGuiSize(int size);

    [[nodiscard]] int getGuiSize() const { return gui_size; }
    [[nodiscard]] float getPixelScaleFactor() const { return physicalPixelScaleFactor; }
    void setPixelScaleFactor(const float factor) { physicalPixelScaleFactor = factor; }

    // Patch Management
    void scanAndUpdatePatchList();

    // Zoom
    void setDefaultZoomFactor(float f);
    float getDefaultZoomFactor() const;

    // Software Renderer (windows only)
    void setUseSoftwareRenderer(bool b);
    bool getUseSoftwareRenderer() const;

    // Load save and init patch
    bool loadPatch(const PatchTreeNode::ptr_t &fxpFile);
    bool loadPatch(const juce::File &fxpFile);
    bool savePatch(const juce::File &fxpFile);
    void initializePatch() const;

    // copy paste and detect a serialized FXP onto clipboard
    void copyPatch();
    void pastePatch();
    bool isPatchInClipboard();

    juce::File getPrecsetsFolder() const { return getDocumentFolder().getChildFile("Patches"); }

    // callbacks
    std::function<void(int idx)> hostUpdateCallback;
    std::function<bool(juce::MemoryBlock &)> loadMemoryBlockCallback;
    std::function<void(juce::MemoryBlock &)> getProgramStateInformation;
    std::function<void(char *, int)> copyTruncatedProgramNameToFXPBuffer;
    std::function<void()> resetPatchToDefault;

    juce::File fsPathToJuceFile(const fs::path &) const;
    fs::path juceFileToFsPath(const juce::File &) const;

    void setPluginAPIScale(float s) { pluginApiScale = s; }
    float getPluginAPIScale() const { return pluginApiScale; }

    void setLastPatchAuthor(const juce::String &);
    juce::String getLastPatchAuthor() const;
    void setLastPatchLicense(const juce::String &);
    juce::String getLastPatchLicense() const;

  private:
    float pluginApiScale{1.f};
    // Config Management
    std::unique_ptr<juce::PropertiesFile> config;
    juce::InterProcessLock configLock;

    LocationType resolvedFactoryLocationType{SYSTEM_FACTORY};

    void updateConfig();
    bool serializePatchAsFXPOnto(juce::MemoryBlock &memoryBlock) const;
    bool isMemoryBlockAPatch(const juce::MemoryBlock &mb);

    // File Collections
    std::vector<ThemeLocation> themeLocations;
    std::vector<MidiLocation> midiLocations;

    // Current States
    ThemeLocation currentTheme;
    MidiLocation currentMidi;

    int gui_size{};
    float physicalPixelScaleFactor{};

    // patch
    juce::String currentPatch;
    juce::File currentPatchFile;
};

#endif // OBXF_SRC_UTILS_H
