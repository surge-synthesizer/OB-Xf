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

#include "Utils.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/plugininfra/paths.h"
#include "sst/plugininfra/strnatcmp.h"
#include "Utils.h"

Utils::Utils() : configLock("__" JucePlugin_Name "ConfigLock__")
{
#if DEBUG
    if (fromHumanReadableVersion(humanReadableVersion(currentStreamingVersion)) !=
        currentStreamingVersion)
    {
        throw std::runtime_error("Invalid version number conversion");
    }
#endif

    createDocumentFolderIfMissing();
    resolveFactoryFolderInUse();

    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;
    options.processLock = &configLock;
    config = std::make_unique<juce::PropertiesFile>(
        getDocumentFolder().getChildFile("settings.xml"), options);
    gui_size = config->getIntValue("gui_size", 1);

    // FOR NOW - fix this to be factory in future
    auto tl = (LocationType)config->getIntValue("theme_loc", 1);
    auto tn = config->getValue("theme_name", "Default");
    auto tf = getThemeFolderFor(tl).getChildFile(tn);
    currentTheme = {tl, tn, tf};

    // std::cout << "[Utils::Utils] Current theme: " << currentTheme.toStdString() << std::endl;
    scanAndUpdateThemes();
    rescanPatchTree();

    if (themeLocations.size() > 0 && !currentTheme.file.exists() &&
        currentTheme.locationType != EMBEDDED)
    {
        DBG("Replacing theme with first in list");
        currentTheme = themeLocations[0];
    }
}

Utils::~Utils()
{
    if (config)
        config->saveIfNeeded();
    config = nullptr;
}

juce::File Utils::fsPathToJuceFile(const fs::path &p) const
{
#if JUCE_WINDOWS
    return juce::File(juce::String((wchar_t *)p.u16string().c_str()));
#else
    return juce::File(p.u8string());
#endif
}

fs::path Utils::juceFileToFsPath(const juce::File &f) const
{
    auto js = f.getFullPathName();
#if JUCE_WINDOWS
    return fs::path(js.toUTF16().getAddress());
#else
    return fs::path(fs::u8path(js.toUTF8().getAddress()));
#endif
}

juce::File Utils::getSystemFactoryFolder() const
{
    auto pt =
        sst::plugininfra::paths::bestLibrarySharedVendorFolderPathFor("Surge Synth Team", "OB-Xf");
    return fsPathToJuceFile(pt);
}

juce::File Utils::getLocalFactoryFolder() const
{
    auto pt = sst::plugininfra::paths::bestLibrarySharedVendorFolderPathFor("Surge Synth Team",
                                                                            "OB-Xf", true);
    return fsPathToJuceFile(pt);
}

juce::File Utils::getDocumentFolder() const
{
#if JUCE_WINDOWS
    char *envvar = nullptr;
    size_t sz = 0;
    _dupenv_s(&envvar, &sz, "OBXF_DOCUMENT_FOLDER");
#else
    const char *envvar = std::getenv("OBXF_DOCUMENT_FOLDER");
#endif

    if (envvar != nullptr && envvar != 0)
    {
#if JUCE_WINDOWS
        std::string result(envvar, sz);
        free(envvar);
#else
        std::string result(envvar);
#endif

        return juce::File(result).getChildFile("Surge Synth Team").getChildFile("OB-Xf");
    }

    auto path =
        sst::plugininfra::paths::bestDocumentsVendorFolderPathFor("Surge Synth Team", "OB-Xf");
    auto folder = fsPathToJuceFile(path);

    if (folder.isSymbolicLink())
    {
        folder = folder.getLinkedTarget();
    }

    return folder;
}

const std::vector<Utils::MidiLocation> &Utils::getMidiLocations() const { return midiLocations; }
Utils::MidiLocation Utils::getCurrentMidiLocation() const { return currentMidi; }

void Utils::setCurrentMidiLocation(const Utils::MidiLocation &loc)
{
    currentMidi = loc;
    config->setValue("midi_loc", (int)loc.locationType);
    config->setValue("midi_name", loc.dirName);
    config->setNeedsToBeSaved(true);
}

juce::File Utils::getMidiFolderFor(LocationType loc) const
{
    switch (loc)
    {
    case SYSTEM_FACTORY:
        return getSystemFactoryFolder().getChildFile("MIDI");
    case LOCAL_FACTORY:
        return getLocalFactoryFolder().getChildFile("MIDI");
    case USER:
    default:
        break;
    }
    return getDocumentFolder().getChildFile("MIDI");
}

std::vector<juce::File> Utils::getThemeFolders() const
{
    return {getFactoryFolderInUse(), getThemeFolderFor(USER)};
}

juce::File Utils::getThemeFolderFor(LocationType loc) const
{
    switch (loc)
    {
    case SYSTEM_FACTORY:
        return getSystemFactoryFolder().getChildFile("Themes");
    case LOCAL_FACTORY:
        return getLocalFactoryFolder().getChildFile("Themes");
    case USER:
    default:
        break;
    }
    return getDocumentFolder().getChildFile("Themes");
}

Utils::ThemeLocation Utils::getCurrentThemeLocation() const
{
    // DBG(" THEME : " << currentTheme);
    return currentTheme;
}

const std::vector<Utils::ThemeLocation> &Utils::getThemeLocations() const { return themeLocations; }

void Utils::setCurrentThemeLocation(const Utils::ThemeLocation &loc)
{
    currentTheme = loc;

    config->setValue("theme_loc", (int)loc.locationType);
    config->setValue("theme_name", loc.dirName);
    config->setNeedsToBeSaved(true);
}

void Utils::setGuiSize(const int gui_size)
{
    this->gui_size = gui_size;
    config->setValue("gui_size", gui_size);
    config->setNeedsToBeSaved(true);
}

void Utils::scanAndUpdateThemes()
{
    themeLocations.clear();

    for (auto &t : {resolvedFactoryLocationType, LocationType::EMBEDDED, LocationType::USER})
    {
        if (t == LocationType::EMBEDDED)
        {
            themeLocations.emplace_back(t, embeddedThemeSentinel.getFileName(),
                                        embeddedThemeSentinel);
        }
        else
        {
            auto dir = getThemeFolderFor(t);
            for (const auto &entry :
                 juce::RangedDirectoryIterator(dir, false, "*", juce::File::findDirectories))
            {
                themeLocations.emplace_back(t, entry.getFile().getFileName(), entry.getFile());
            }
        }
    }
}

bool Utils::loadPatch(const PatchTreeNode::ptr_t &fxpFile)
{
    auto res = loadPatch(fxpFile->file);
    if (res && hostUpdateCallback)
    {
        hostUpdateCallback(fxpFile->index);
    }

    return res;
}
bool Utils::loadPatch(const juce::File &fxpFile)
{
    OBLOG(patches, "Load from fxpfile '" << fxpFile.getFullPathName() << "'");
    juce::MemoryBlock mb;

    if (!fxpFile.loadFileAsData(mb))
        return false;

    if (loadMemoryBlockCallback)
    {
        if (!loadMemoryBlockCallback(mb)) // todo make it respond to any program number
            return false;
    }

    currentPatch = fxpFile.getFileName();
    currentPatchFile = fxpFile;

    return true;
}

bool Utils::serializePatchAsFXPOnto(juce::MemoryBlock &memoryBlock) const
{
    if (getProgramStateInformation)
    {
        juce::MemoryBlock m;

        getProgramStateInformation(m);

        memoryBlock.reset();
        const auto totalLen = sizeof(fxProgramSet) + m.getSize() - 8;
        memoryBlock.setSize(totalLen, true);

        const auto set = static_cast<fxProgramSet *>(memoryBlock.getData());
        set->chunkMagic = fxbName("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName("FPCh");
        set->version = fxbSwap(fxbVersionNum);
        set->fxID = fxbName("OBXf");
        set->fxVersion = fxbSwap(fxbVersionNum);

        set->numPrograms = 1;

        if (copyTruncatedProgramNameToFXPBuffer)
            copyTruncatedProgramNameToFXPBuffer(set->name, 28);

        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());

        return true;
    }

    return false;
}

bool Utils::savePatch(const juce::File &fxpFile)
{
    juce::MemoryBlock memoryBlock;

    if (serializePatchAsFXPOnto(memoryBlock))
    {
        if (!fxpFile.createDirectory())
            return false;
        fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
        currentPatch = fxpFile.getFileName();
        currentPatchFile = fxpFile;

        rescanPatchTree();
        return true;
    }
    return false;
}

void Utils::initializePatch() const
{
    OBLOG(patches, "Initialize patch");
    if (resetPatchToDefault)
        resetPatchToDefault();

    if (hostUpdateCallback)
    {
        hostUpdateCallback(-1);
    }
}

void Utils::copyPatch()
{
    juce::MemoryBlock serializedData;
    if (serializePatchAsFXPOnto(serializedData))
    {
        juce::SystemClipboard::copyTextToClipboard(serializedData.toBase64Encoding());
    }
}

void Utils::pastePatch()
{
    if (loadMemoryBlockCallback)
    {
        juce::MemoryBlock memoryBlock;
        if (memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard()))
            loadMemoryBlockCallback(memoryBlock);
    }
}

bool Utils::isPatchInClipboard()
{
    juce::MemoryBlock memoryBlock;
    if (!memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard()))
        return false;

    return isMemoryBlockAPatch(memoryBlock);
}

bool Utils::isMemoryBlockAPatch(const juce::MemoryBlock &mb)
{
    const void *const data = mb.getData();
    const size_t dataSize = mb.getSize();

    if (dataSize < 28)
        return false;

    if (const fxSet *const set = static_cast<const fxSet *>(data);
        (!compareMagic(set->chunkMagic, "CcnK")) || fxbSwap(set->version) > fxbVersionNum)
        return false;
    return true;
}

void Utils::setDefaultZoomFactor(float f) { config->setValue("default_zoom", f); }
float Utils::getDefaultZoomFactor() const { return config->getDoubleValue("default_zoom", 1.0); }

void Utils::setUseSoftwareRenderer(bool f) { config->setValue("use_sw_rend", f); }
bool Utils::getUseSoftwareRenderer() const { return config->getBoolValue("use_sw_rend", false); }

void Utils::createDocumentFolderIfMissing()
{
    auto docFolder = getDocumentFolder();

    if (!docFolder.isDirectory())
    {
        docFolder.createDirectory();
    }

    for (const auto &p : {"Patches", "Themes", "Patches", "MIDI"})
    {
        auto subFolder = docFolder.getChildFile(p);
        if (!subFolder.isDirectory())
        {
            subFolder.createDirectory();
        }
    }
}

void Utils::resolveFactoryFolderInUse()
{
    auto dL = getLocalFactoryFolder();

    if (dL.isDirectory())
    {
        DBG("Using 'local' factory folder");
        resolvedFactoryLocationType = LOCAL_FACTORY;
        return;
    }

    auto dS = getSystemFactoryFolder();

    if (!dS.isDirectory())
    {
        DBG("Using 'system' factory folder, but its not there");
    }
    resolvedFactoryLocationType = SYSTEM_FACTORY;
}

juce::File Utils::getFactoryFolderInUse() const
{
    if (resolvedFactoryLocationType == LOCAL_FACTORY)
    {
        return getLocalFactoryFolder();
    }

    return getSystemFactoryFolder();
}

// patch management new code

juce::File Utils::getPatchFolderFor(LocationType loc) const
{
    switch (loc)
    {
    case SYSTEM_FACTORY:
        return getSystemFactoryFolder().getChildFile("Patches");
    case LOCAL_FACTORY:
        return getLocalFactoryFolder().getChildFile("Patches");
    case USER:
    default:
        break;
    }
    return getDocumentFolder().getChildFile("Patches");
}

void Utils::rescanPatchTree()
{
    patchesAsLinearList.clear();
    lastFactoryPatch = 0;

    patchRoot = std::make_shared<PatchTreeNode>();
    patchRoot->isFolder = true;
    patchRoot->children.clear();
    patchRoot->displayName = "root";
    patchRoot->locationType = LocationType::EMBEDDED;

    for (auto &f : {SYSTEM_FACTORY, LOCAL_FACTORY, USER})
    {
        auto fl = getPatchFolderFor(f);
        if (fl.isDirectory())
        {
            PatchTreeNode::ptr_t pt = std::make_shared<PatchTreeNode>();
            pt->isFolder = true;
            pt->locationType = f;
            pt->displayName = "Root";
            scanPatchFolderInto(pt, f, fl);
            patchRoot->children.push_back(std::move(pt));
        }
    }

    auto applyRec = [](PatchTreeNode::ptr_t node,
                       const std::function<bool(const PatchTreeNode::ptr_t &)> &f,
                       auto &&self) -> void {
        if (!f(node))
        {
            for (auto &child : node->children)
            {
                self(child, f, self);
            }
        }
    };

    int idx{0};
    // First index every root note
    applyRec(
        patchRoot,
        [this, &idx](auto node) -> bool {
            if (!node->isFolder)
            {
                node->index = idx++;
                if (node->locationType == LocationType::SYSTEM_FACTORY ||
                    node->locationType == LocationType::LOCAL_FACTORY)
                    lastFactoryPatch = idx;
                return true;
            }
            return false;
        },
        applyRec);

    // Now we need to know the total range of all my children, recursively.
    // This is like an AP CS test. Wonder if I can get it right first time?
    // (Answer: I did! Yay!)
    auto childRange = [](const PatchTreeNode::ptr_t &node, auto &&self) -> void {
        if (!node->isFolder)
        {
            node->childRange = {node->index, node->index};
        }
        else
        {
            int start{std::numeric_limits<int>::max()};
            int end{std::numeric_limits<int>::min()};
            for (auto &c : node->children)
            {
                if (!c->isFolder)
                    node->nonFolderChildIndices.push_back(c->index);
                self(c, self);
                auto [lo, hi] = c->childRange;
                start = std::min(lo, start);
                end = std::max(hi, end);
            }
            node->childRange = {start, end};
        }
    };
    childRange(patchRoot, childRange);

    auto idxInParent = [](const PatchTreeNode::ptr_t node, auto &&self) -> void {
        int idx{0};
        for (auto &c : node->children)
        {
            if (c->isFolder)
            {
                self(c, self);
            }
            else
            {
                c->indexInParent = idx++;
            }
        }
    };
    idxInParent(patchRoot, idxInParent);

    // Finally collect just the root nodes into a linear list copy
    applyRec(
        patchRoot,
        [this](auto node) -> bool {
            if (!node->isFolder)
            {
                patchesAsLinearList.push_back(node);
                return true;
            }
            return false;
        },
        applyRec);

    patchRoot->print();
}

void Utils::scanPatchFolderInto(const PatchTreeNode::ptr_t &parent, LocationType lt,
                                juce::File &folder)
{
    for (auto &kid : folder.findChildFiles(juce::File::findFilesAndDirectories, false))
    {
        if (kid.isDirectory())
        {
            PatchTreeNode::ptr_t pt = std::make_shared<PatchTreeNode>();
            pt->parent = parent;
            pt->locationType = lt;
            pt->displayName = kid.getFileName();
            pt->isFolder = true;
            parent->children.push_back(std::move(pt));
            scanPatchFolderInto(parent->children.back(), lt, kid);
        }
        if (kid.getFileExtension().toLowerCase() == ".fxp")
        {
            PatchTreeNode::ptr_t pt = std::make_shared<PatchTreeNode>();
            pt->parent = parent;
            pt->locationType = lt;
            pt->displayName = kid.getFileNameWithoutExtension();
            pt->isFolder = false;
            pt->file = kid;
            parent->children.push_back(std::move(pt));
        }
    }

    // TODO sort
    std::sort(parent->children.begin(), parent->children.end(), [](const auto &a, const auto &b) {
        if (a->locationType != b->locationType)
            return (int)a->locationType < (int)b->locationType;
        if (a->isFolder != b->isFolder)
            return a->isFolder;

        return strnatcasecmp(a->displayName.toRawUTF8(), b->displayName.toRawUTF8()) < 0;
    });
}

void Utils::setLastPatchAuthor(const juce::String &a)
{
    config->setValue("last_patch_author", a);
    config->setNeedsToBeSaved(true);
}
juce::String Utils::getLastPatchAuthor() const
{
    return config->getValue("last_patch_author", "Author Unknown");
}
void Utils::setLastPatchLicense(const juce::String &a)
{
    config->setValue("last_patch_license", a);
    config->setNeedsToBeSaved(true);
}
juce::String Utils::getLastPatchLicense() const
{
    return config->getValue("last_patch_license", "CC0 / Public Domain");
}