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
#include "Utils.h"

Utils::Utils() : configLock("__" JucePlugin_Name "ConfigLock__")
{
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
    scanAndUpdateBanks();
    scanAndUpdateThemes();
    if (bankLocations.size() > 0)
    {
        loadFromFXBFile(bankLocations[0].file);
    }

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

juce::File Utils::getBanksFolderFor(LocationType loc) const
{
    switch (loc)
    {
    case SYSTEM_FACTORY:
        return getSystemFactoryFolder().getChildFile("Banks");
    case LOCAL_FACTORY:
        return getLocalFactoryFolder().getChildFile("Banks");
    case USER:
    default:
        break;
    }
    return getDocumentFolder().getChildFile("Banks");
}

Utils::ThemeLocation Utils::getCurrentThemeLocation() const
{
    // DBG(" THEME : " << currentTheme);
    return currentTheme;
}

const std::vector<Utils::ThemeLocation> &Utils::getThemeLocations() const { return themeLocations; }

const std::vector<Utils::BankLocation> &Utils::getBankLocations() const { return bankLocations; }

Utils::BankLocation Utils::getCurrentBankLocation() const { return currentBank; }

void Utils::setCurrentThemeLocation(const Utils::ThemeLocation &loc)
{
    currentTheme = loc;

    config->setValue("theme_loc", (int)loc.locationType);
    config->setValue("theme_name", loc.dirName);
    config->setNeedsToBeSaved(true);
}

std::vector<juce::File> Utils::getBanksFolders() const
{
    return {getFactoryFolderInUse().getChildFile("Banks"),
            getDocumentFolder().getChildFile("Banks")};
}

void Utils::setGuiSize(const int gui_size)
{
    this->gui_size = gui_size;
    config->setValue("gui_size", gui_size);
    config->setNeedsToBeSaved(true);
}

bool Utils::loadFromFXBLocation(const BankLocation &fxbFile)
{
    auto res = loadFromFXBFile(fxbFile.file);
    if (res)
    {
        currentBank = fxbFile;
    }
    return res;
}

bool Utils::loadFromFXBFile(const juce::File &fxbFile)
{
    juce::MemoryBlock mb;
    if (!fxbFile.loadFileAsData(mb))
        return false;

    if (loadMemoryBlockCallback)
    {
        if (!loadMemoryBlockCallback(mb, -1))
            return false;
    }

    currentBank = {LocationType::USER, fxbFile.getFileName(), fxbFile};

    // use this instead of directly using previous method updateHostDisplay();
    if (hostUpdateCallback)
        hostUpdateCallback();

    return true;
}

void Utils::scanAndUpdateBanks()
{
    bankLocations.clear();

    for (auto &t : {resolvedFactoryLocationType, LocationType::USER})
    {
        auto dir = getBanksFolderFor(t);
        for (const auto &entry :
             juce::RangedDirectoryIterator(dir, false, "*.fxb", juce::File::findFiles))
        {
            bankLocations.emplace_back(t, entry.getFile().getFileName(), entry.getFile());
        }
    }
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

void Utils::saveBank() const
{
    if (!saveFXBFile(currentBank.file))
    {
        DBG("Failed to save the bank file.");
    }
}

bool Utils::saveBank(const juce::File &fxbFile)
{
    if (!saveFXBFile(fxbFile))
    {
        DBG("Failed to save the bank file.");
        return false;
    }

    scanAndUpdateBanks();
    currentBank = {LocationType::USER, fxbFile.getFileName(), fxbFile};
    return true;
}

bool Utils::saveFXBFile(const juce::File &fxbFile) const
{
    juce::MemoryBlock m;
    if (getStateInformationCallback)
    {
        getStateInformationCallback(m);
        juce::MemoryBlock memoryBlock;
        memoryBlock.reset();
        const auto totalLen = sizeof(fxChunkSet) + m.getSize() - 8;
        memoryBlock.setSize(totalLen, true);

        const auto set = static_cast<fxChunkSet *>(memoryBlock.getData());
        set->chunkMagic = fxbName("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName("FBCh");
        set->version = fxbSwap(fxbVersionNum);
        set->fxID = fxbName("OBXf");
        set->fxVersion = fxbSwap(fxbVersionNum);

        if (getNumProgramsCallback)
            set->numPrograms = fxbSwap(getNumProgramsCallback());

        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());
        fxbFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }

    return true;
}

bool Utils::loadFromFXPFile(const juce::File &fxpFile)
{
    juce::MemoryBlock mb;

    if (!fxpFile.loadFileAsData(mb))
        return false;

    if (loadMemoryBlockCallback)
    {
        if (!loadMemoryBlockCallback(mb, -1)) // todo make it respond to any program number
            return false;
    }

    currentPatch = fxpFile.getFileName();
    currentPatchFile = fxpFile;

    if (hostUpdateCallback)
        hostUpdateCallback();

    return true;
}

bool Utils::loadPatch(const juce::File &fxpFile)
{
    const bool success = loadFromFXPFile(fxpFile);
    return success;
}

juce::MemoryBlock Utils::serializePatch(juce::MemoryBlock &memoryBlock, const int index) const
{
    juce::MemoryBlock m;

    if (getProgramStateInformation)
    {
        getProgramStateInformation(index, m);

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

        if (getNumPrograms)
            set->numPrograms = fxbSwap(getNumPrograms());

        if (copyProgramNameToBuffer)
            copyProgramNameToBuffer(index, set->name, 28);

        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());

        return memoryBlock;
    }

    return m;
}

bool Utils::saveFXPFile(const juce::File &fxpFile) const
{
    juce::MemoryBlock m;

    if (getProgramStateInformation)
    {
        getProgramStateInformation(-1, m);
        juce::MemoryBlock memoryBlock;
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

        if (getNumPrograms)
            set->numPrograms = fxbSwap(getNumPrograms());

        if (copyProgramNameToBuffer)
            copyProgramNameToBuffer(-1, set->name, 28);

        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());

        fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }

    return true;
}

bool Utils::saveFXPFileFrom(const juce::File &fxpFile, const int index) const
{
    juce::MemoryBlock m;

    if (getProgramStateInformation)
    {
        getProgramStateInformation(index, m);
        juce::MemoryBlock memoryBlock;
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

        if (getNumPrograms)
            set->numPrograms = fxbSwap(getNumPrograms());

        if (copyProgramNameToBuffer)
            copyProgramNameToBuffer(index, set->name, 28);

        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());

        fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }

    return true;
}

bool Utils::savePatch(const juce::File &fxpFile)
{
    const bool success = saveFXPFile(fxpFile);
    if (success)
    {
        currentPatch = fxpFile.getFileName();
        currentPatchFile = fxpFile;
    }
    return success;
}

bool Utils::savePatchFrom(const juce::File &fxpFile, const int index)
{
    const bool success = saveFXPFileFrom(fxpFile, index);
    return success;
}

void Utils::changePatchName(const juce::String &name) const
{
    if (setPatchName)
        setPatchName(name);
}

void Utils::initializePatch() const
{
    if (resetPatchToDefault)
        resetPatchToDefault();

    if (setPatchName)
        setPatchName(INIT_PATCH_NAME);

    if (sendChangeMessage)
        sendChangeMessage();
}

void Utils::savePatch() { savePatch(currentPatchFile); }

void Utils::copyPatch(const int index)
{
    juce::MemoryBlock serializedData;
    serializePatch(serializedData, index);
    juce::SystemClipboard::copyTextToClipboard(serializedData.toBase64Encoding());
}

void Utils::pastePatch(const int index)
{
    if (pastePatchCallback)
    {
        juce::MemoryBlock memoryBlock;
        memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());
        pastePatchCallback(memoryBlock, index);
    }
}

bool Utils::isPatchInClipboard()
{
    juce::MemoryBlock memoryBlock;
    memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());

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

    for (const auto &p : {"Patches", "Themes", "Banks", "MIDI"})
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