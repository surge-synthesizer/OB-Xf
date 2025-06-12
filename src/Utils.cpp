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

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utils.h"

Utils::Utils() : configLock("__" JucePlugin_Name "ConfigLock__")
{
    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;
    options.processLock = &configLock;
    config = std::make_unique<juce::PropertiesFile>(getDocumentFolder().getChildFile("Skin.xml"),
                                                    options);
    gui_size = config->getIntValue("gui_size", 1);
    currentSkin = config->containsKey("skin") ? config->getValue("skin") : "Ilkka Rosma Dark";

    // std::cout << "[Utils::Utils] Current skin: " << currentSkin.toStdString() << std::endl;
    currentBank = "rfawcett160 bank";
    scanAndUpdateBanks();
    scanAndUpdateSkins();
    if (bankFiles.size() > 0)
    {
        loadFromFXBFile(bankFiles[0]);
    }
}

Utils::~Utils()
{
    if (config)
        config->saveIfNeeded();
    config = nullptr;
}

juce::File Utils::getDocumentFolder() const
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("Surge Synth Team")
                            .getChildFile("OB-Xf");
    if (folder.isSymbolicLink())
        folder = folder.getLinkedTarget();
    return folder;
}

juce::File Utils::getMidiFolder() const { return getDocumentFolder().getChildFile("MIDI"); }

juce::File Utils::getSkinFolder() const { return getDocumentFolder().getChildFile("Themes"); }

juce::File Utils::getCurrentSkinFolder() const
{
    // DBG(" SKIN : " << currentSkin);
    return getSkinFolder().getChildFile(currentSkin);
}

const juce::Array<juce::File> &Utils::getSkinFiles() const { return skinFiles; }

const juce::Array<juce::File> &Utils::getBankFiles() const { return bankFiles; }

juce::File Utils::getCurrentBankFile() const { return getBanksFolder().getChildFile(currentBank); }

void Utils::setCurrentSkinFolder(const juce::String &folderName)
{
    currentSkin = folderName;

    config->setValue("skin", folderName); // SUBCLASS CONFIG
    config->setNeedsToBeSaved(true);
}

juce::File Utils::getBanksFolder() const { return getDocumentFolder().getChildFile("Banks"); }

void Utils::setGuiSize(const int gui_size)
{
    this->gui_size = gui_size;
    config->setValue("gui_size", gui_size);
    config->setNeedsToBeSaved(true);
}

bool Utils::loadFromFXBFile(const juce::File &fxbFile)
{
    juce::MemoryBlock mb;
    if (!fxbFile.loadFileAsData(mb))
        return false;

    if (loadMemoryBlockCallback)
    {
        if (!loadMemoryBlockCallback(mb))
            return false;
    }

    currentBank = fxbFile.getFileName();
    currentBankFile = fxbFile;

    // use this instead of directly using previous method updateHostDisplay();
    if (hostUpdateCallback)
        hostUpdateCallback();

    return true;
}

void Utils::scanAndUpdateBanks()
{
    bankFiles.clear();

    for (const auto &entry :
         juce::RangedDirectoryIterator(getBanksFolder(), false, "*.fxb", juce::File::findFiles))
    {
        bankFiles.addUsingDefaultSort(entry.getFile());
        //  DBG("Scan Banks: " << entry.getFile().getFullPathName());
    }
}

void Utils::scanAndUpdateSkins()
{
    skinFiles.clearQuick();

    for (const auto &entry :
         juce::RangedDirectoryIterator(getSkinFolder(), false, "*", juce::File::findDirectories))
    {
        skinFiles.addUsingDefaultSort(entry.getFile());
    }
}

bool Utils::deleteBank()
{
    if (currentBankFile.deleteFile())
    {
        scanAndUpdateBanks();
        if (bankFiles.size() > 0)
        {
            return loadFromFXBFile(bankFiles[0]);
        }
        return true;
    }
    return false;
}

void Utils::saveBank() const
{
    if (!saveFXBFile(currentBankFile))
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
    currentBankFile = fxbFile;
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
        set->fxID = fxbName("Obxf");
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
        if (!loadMemoryBlockCallback(mb))
            return false;
    }

    currentPreset = fxpFile.getFileName();
    currentPresetFile = fxpFile;
    if (hostUpdateCallback)
        hostUpdateCallback();

    return true;
}

bool Utils::loadPreset(const juce::File &fxpFile)
{
    loadFromFXPFile(fxpFile);
    currentPreset = fxpFile.getFileName();
    currentPresetFile = fxpFile;
    return true;
}

void Utils::serializePreset(juce::MemoryBlock &memoryBlock) const
{
    juce::MemoryBlock m;
    if (getCurrentProgramStateInformation)
    {
        getCurrentProgramStateInformation(m);
        memoryBlock.reset();
        const auto totalLen = sizeof(fxProgramSet) + m.getSize() - 8;
        memoryBlock.setSize(totalLen, true);

        const auto set = static_cast<fxProgramSet *>(memoryBlock.getData());
        set->chunkMagic = fxbName("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName("FPCh");
        set->version = fxbSwap(fxbVersionNum);
        set->fxID = fxbName("Obxf");
        set->fxVersion = fxbSwap(fxbVersionNum);
        if (getNumPrograms)
            set->numPrograms = fxbSwap(getNumPrograms());
        if (copyProgramNameToBuffer)
            copyProgramNameToBuffer(set->name, 28);
        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());
    }
}

bool Utils::saveFXPFile(const juce::File &fxpFile) const
{
    juce::MemoryBlock m;
    if (getCurrentProgramStateInformation)
    {
        getCurrentProgramStateInformation(m);
        juce::MemoryBlock memoryBlock;
        memoryBlock.reset();
        const auto totalLen = sizeof(fxProgramSet) + m.getSize() - 8;
        memoryBlock.setSize(totalLen, true);

        const auto set = static_cast<fxProgramSet *>(memoryBlock.getData());
        set->chunkMagic = fxbName("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName("FPCh");
        set->version = fxbSwap(fxbVersionNum);
        set->fxID = fxbName("Obxf");
        set->fxVersion = fxbSwap(fxbVersionNum);
        if (getNumPrograms)
            set->numPrograms = fxbSwap(getNumPrograms());
        if (copyProgramNameToBuffer)
            copyProgramNameToBuffer(set->name, 28);
        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());

        fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }
    return true;
}

bool Utils::savePreset(const juce::File &fxpFile)
{
    const bool success = saveFXPFile(fxpFile);
    if (success)
    {
        currentPreset = fxpFile.getFileName();
        currentPresetFile = fxpFile;
    }
    return success;
}

void Utils::changePresetName(const juce::String &name) const
{
    if (setProgramName)
        setProgramName(name);
}

void Utils::initializePreset() const
{
    if (resetProgramToDefault)
        resetProgramToDefault();

    if (setProgramName)
        setProgramName("Default");

    if (sendChangeMessage)
        sendChangeMessage();
}

void Utils::newPreset(const juce::String &name) const
{
    if (getNumPrograms && isProgramNameCallback && setCurrentProgram && setProgramName)
    {
        const int count = getNumPrograms();
        for (int i = 0; i < count; ++i)
        {
            if (isProgramNameCallback(i, name))
            {
                setCurrentProgram(i);
                return;
            }
        }
        setCurrentProgram(0);
        setProgramName(name);
    }
}

void Utils::savePreset() { savePreset(currentPresetFile); }

bool Utils::isMemoryBlockAPreset(const juce::MemoryBlock &mb)
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
