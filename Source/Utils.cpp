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

    //showPresetBar = config->getBoolValue("presetnavigation");
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