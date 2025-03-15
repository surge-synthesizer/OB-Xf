#include "Utils.h"
#include <juce_gui_basics/juce_gui_basics.h>

Utils::Utils() : configLock("__" JucePlugin_Name "ConfigLock__")
{
    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 2500;
    options.processLock = &configLock;
    config = std::make_unique<juce::PropertiesFile>(getDocumentFolder().getChildFile("Skin.xml"),
                                                    options);

    showPresetBar = config->getBoolValue("presetnavigation");
    gui_size = config->getIntValue("gui_size", 1);
    tooltipBehavior = static_cast<Tooltip>(config->getIntValue("tooltip", 1));
    currentSkin = config->containsKey("skin") ? config->getValue("skin") : "Ilkka Rosma Dark";

    std::cout << "[Utils::Utils] Current skin: " << currentSkin.toStdString() << std::endl;
    currentBank = "000 - FMR OB-Xa Patch Book";
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
        .getChildFile("discoDSP")
        .getChildFile("OB-Xd");
    if (folder.isSymbolicLink())
        folder = folder.getLinkedTarget();
    return folder;
}

juce::File Utils::getMidiFolder() const
{
    return getDocumentFolder().getChildFile("MIDI");
}

juce::File Utils::getSkinFolder() const
{
    return getDocumentFolder().getChildFile("Themes");
}

juce::File Utils::getCurrentSkinFolder() const
{
    DBG(" SKIN : " << currentSkin);
    return getSkinFolder().getChildFile(currentSkin);
}

const juce::Array<juce::File> &Utils::getSkinFiles() const
{
    return skinFiles;
}

const juce::Array<juce::File> &Utils::getBankFiles() const
{
    return bankFiles;
}

juce::File Utils::getCurrentBankFile() const
{
    return getBanksFolder().getChildFile(currentBank);
}

void Utils::setCurrentSkinFolder(const juce::String &folderName)
{
    currentSkin = folderName;

    config->setValue("skin", folderName); //SUBCLASS CONFIG
    config->setNeedsToBeSaved(true);
}


juce::File Utils::getBanksFolder() const
{
    return getDocumentFolder().getChildFile("Banks");
}


void Utils::openInPdf(const juce::File &file)
{
    if (file.existsAsFile())
    {
        juce::File absoluteFile = file.getFullPathName();

#if JUCE_WINDOWS
        const juce::String command = R"(cmd /c start "" ")" + absoluteFile.getFullPathName() + "\"";
        system(command.toRawUTF8());
#else
        absoluteFile.startAsProcess();
#endif
    }
    else
    {
        juce::NativeMessageBox::showMessageBox(
            juce::AlertWindow::WarningIcon,
            "Error",
            "OB-Xd Manual.pdf not found."
            );
    }
}

void Utils::setGuiSize(const int gui_size)
{
    this->gui_size = gui_size;
    config->setValue("gui_size", gui_size);
    config->setNeedsToBeSaved(true);
}

Tooltip Utils::getTooltipBehavior() const
{
    return tooltipBehavior;
}

void Utils::setTooltipBehavior(const Tooltip tooltip)
{
    this->tooltipBehavior = tooltip;
    config->setValue("tooltip", static_cast<int>(tooltip));
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

    for (const auto &entry : juce::RangedDirectoryIterator(getBanksFolder(), false, "*.fxb",
                                                           juce::File::findFiles))
    {
        bankFiles.addUsingDefaultSort(entry.getFile());
        DBG("Scan Banks: " << entry.getFile().getFullPathName());
    }
}

void Utils::scanAndUpdateSkins()
{
    skinFiles.clearQuick();

    for (const auto &entry : juce::RangedDirectoryIterator(getSkinFolder(), false, "*",
                                                           juce::File::findDirectories))
    {
        skinFiles.addUsingDefaultSort(entry.getFile());
    }
}

bool Utils::deleteBank()
{
    currentBankFile.deleteFile();
    scanAndUpdateBanks();
    if (bankFiles.size() > 0)
    {
        loadFromFXBFile(bankFiles[0]);
    }
    return true;
}

void Utils::saveBank() const
{
    saveFXBFile(currentBankFile);
}

bool Utils::saveBank(const juce::File &fxbFile)
{
    saveFXBFile(fxbFile);
    currentBankFile = fxbFile;
    return true;
}


bool Utils::saveFXBFile(const juce::File &fxbFile) const
{
    juce::MemoryBlock m;
    if (getStateInformationCallback)
        getStateInformationCallback(m); {
        juce::MemoryBlock memoryBlock;
        memoryBlock.reset();
        const auto totalLen = sizeof(fxChunkSet) + m.getSize() - 8;
        memoryBlock.setSize(totalLen, true);

        const auto set = static_cast<fxChunkSet *>(memoryBlock.getData());
        set->chunkMagic = fxbName("CcnK");
        set->byteSize = 0;
        set->fxMagic = fxbName("FBCh");
        set->version = fxbSwap(fxbVersionNum);
        set->fxID = fxbName("Obxd");
        set->fxVersion = fxbSwap(fxbVersionNum);
        if (getNumProgramsCallback)
            set->numPrograms = fxbSwap(getNumProgramsCallback());
        set->chunkSize = fxbSwap(static_cast<int32_t>(m.getSize()));

        m.copyTo(set->chunk, 0, m.getSize());
        fxbFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
    }

    return true;
}

bool Utils::loadPreset(const juce::File& fxpFile) {
    //loadFromFXPFile(fxpFile); implement
    currentPreset = fxpFile.getFileName();
    currentPresetFile = fxpFile;
    return true;
}

// void Utils::serializePreset(juce::MemoryBlock& memoryBlock)
// {
//     juce::MemoryBlock m;
//     getCurrentProgramStateInformation(m);
//     {
//         memoryBlock.reset();
//         auto totalLen = sizeof (fxProgramSet) + m.getSize() - 8;
//         memoryBlock.setSize (totalLen, true);
//
//         auto set = static_cast<fxProgramSet*>(memoryBlock.getData());
//         set->chunkMagic = fxbName ("CcnK");
//         set->byteSize = 0;
//         set->fxMagic = fxbName ("FPCh");
//         set->version = fxbSwap (fxbVersionNum);
//         set->fxID = fxbName ("Obxd");
//         set->fxVersion = fxbSwap (fxbVersionNum);
//         set->numPrograms = fxbSwap (getNumPrograms());
//         programs.currentProgramPtr->name.copyToUTF8(set->name, 28);
//         set->chunkSize = fxbSwap (static_cast<int32_t>(m.getSize()));
//
//         m.copyTo (set->chunk, 0, m.getSize());
//     }
// }
//
//
// bool Utils::saveFXPFile(const juce::File& fxpFile){
//     juce::MemoryBlock m, memoryBlock;
//     getCurrentProgramStateInformation(m);
//     {
//         memoryBlock.reset();
//         auto totalLen = sizeof (fxProgramSet) + m.getSize() - 8;
//         memoryBlock.setSize (totalLen, true);
//
//         auto set = static_cast<fxProgramSet*>(memoryBlock.getData());
//         set->chunkMagic = fxbName ("CcnK");
//         set->byteSize = 0;
//         set->fxMagic = fxbName ("FPCh");
//         set->version = fxbSwap (fxbVersionNum);
//         set->fxID = fxbName ("Obxd");
//         set->fxVersion = fxbSwap (fxbVersionNum);
//         set->numPrograms = fxbSwap (getNumPrograms());
//         programs.currentProgramPtr->name.copyToUTF8(set->name, 28);
//         set->chunkSize = fxbSwap (static_cast<int32_t>(m.getSize()));
//
//         m.copyTo (set->chunk, 0, m.getSize());
//
//         fxpFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize());
//     }
//     return true;
// }
//
// bool Utils::savePreset(const juce::File& fxpFile) {
//     saveFXPFile(fxpFile);
//     currentPreset = fxpFile.getFileName();
//     currentPresetFile = fxpFile;
//     return true;
// }
//
// void Utils::changePresetName(const juce::String &name){
//     programs.currentProgramPtr->name = name;
// }
//
// void Utils::deletePreset(){
//     programs.currentProgramPtr->setDefaultValues();
//     programs.currentProgramPtr->name = "Default";
//     sendChangeMessage();
// }
//
// void Utils::newPreset(const juce::String &/*name*/) {
//     for (int i = 0; i < PROGRAMCOUNT; ++i)
//     {
//         if (programs.programs[i].name == "Default"){
//             setCurrentProgram(i);
//             break;
//         }
//     }
// }
//
// void Utils::savePreset() {
//     savePreset(currentPresetFile);
//
// }