#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Constants.h"

class Utils final {
public:

    //Singleton
    static Utils &getInstance() {
        static Utils instance;
        return instance;
    }

    Utils(const Utils &) = delete;

    Utils &operator=(const Utils &) = delete;

    // File System Methods
    static juce::File getDocumentFolder();

    static juce::File getMidiFolder();

    static juce::File getSkinFolder();

    static juce::File getBanksFolder();

    static void openInPdf(const juce::File &file);

    // Skin Management
    [[nodiscard]] const juce::Array<juce::File> &getSkinFiles() const;

    [[nodiscard]] juce::File getCurrentSkinFolder() const;

    void setCurrentSkinFolder(const juce::String &folderName);

    void scanAndUpdateSkins();

    // Bank Management
    [[nodiscard]] const juce::Array<juce::File> &getBankFiles() const;

    [[nodiscard]] juce::File getCurrentBankFile() const;

    // GUI Settings
    void setGuiSize(int size);

    [[nodiscard]] int getGuiSize() const { return gui_size; }
    [[nodiscard]] float getPixelScaleFactor() const { return physicalPixelScaleFactor; }
    void setPixelScaleFactor(const float factor) { physicalPixelScaleFactor = factor; }

    //ToolTips
    [[nodiscard]] Tooltip getTooltipBehavior() const;

    void setTooltipBehavior(Tooltip tooltip);


    //FXB
    bool loadFromFXBFile(const juce::File &fxbFile);

    void scanAndUpdateBanks();

    using HostUpdateCallback = std::function<void()>;

    void setHostUpdateCallback(HostUpdateCallback callback) {
        hostUpdateCallback = std::move(callback);
    }

    bool deleteBank();

    void saveBank();


    // static bool loadFromMemoryBlock(juce::MemoryBlock& mb);
    // static bool restoreProgramSettings(const fxProgram* prog);
    // void setCurrentProgramStateInformation(const void* data, int sizeInBytes);


private:
    Utils();

    ~Utils();

    // Config Management
    std::unique_ptr<juce::PropertiesFile> config;
    juce::InterProcessLock configLock;

    void updateConfig();

    // File Collections
    juce::Array<juce::File> skinFiles;
    juce::Array<juce::File> bankFiles;

    // Current States
    juce::String currentSkin;
    juce::String currentBank;
    int gui_size{};
    float physicalPixelScaleFactor{};
    Tooltip tooltipBehavior;


    juce::File currentBankFile;
    HostUpdateCallback hostUpdateCallback;
};
