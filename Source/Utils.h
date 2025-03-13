#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Constants.h"

class Utils final {
public:
    Utils();

    ~Utils();

    // File System Methods
    [[nodiscard]] juce::File getDocumentFolder() const;

    [[nodiscard]] juce::File getMidiFolder() const;

    [[nodiscard]] juce::File getSkinFolder() const;

    [[nodiscard]] juce::File getBanksFolder() const;

    void openInPdf(const juce::File &file);

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

private:
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
