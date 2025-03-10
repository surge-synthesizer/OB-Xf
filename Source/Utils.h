#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Constants.h"

class Utils final : public juce::ChangeBroadcaster
{
public:
    Utils();
    ~Utils() override;

    // File System Methods
    static juce::File getDocumentFolder();
    static juce::File getMidiFolder();
    static juce::File getSkinFolder();
    static juce::File getBanksFolder();
    static void openInPdf(const juce::File& file);

    // Skin Management
    [[nodiscard]] const juce::Array<juce::File>& getSkinFiles() const;
    [[nodiscard]] juce::File getCurrentSkinFolder() const;
    void setCurrentSkinFolder(const juce::String& folderName);

    // Bank Management
    [[nodiscard]] const juce::Array<juce::File>& getBankFiles() const;
    [[nodiscard]] juce::File getCurrentBankFile() const;

    // GUI Settings
    void setGuiSize(int size);
    [[nodiscard]] int getGuiSize() const { return gui_size; }
    [[nodiscard]] float getPixelScaleFactor() const { return physicalPixelScaleFactor; }
    void setPixelScaleFactor(const float factor) { physicalPixelScaleFactor = factor; }


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
};