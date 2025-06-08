#include "../PluginProcessor.h"
#include "ScaleComponent.h"
#include "BinaryData.h"

ScalableComponent::ScalableComponent(ObxdAudioProcessor *owner_) : processor(owner_) {}

ScalableComponent::~ScalableComponent() = default;

juce::Image ScalableComponent::getScaledImageFromCache(const juce::String &imageName

) const
{
    juce::File skin;
    if (processor)
    {
        if (const juce::File f(processor->getUtils().getCurrentSkinFolder()); f.isDirectory())
            skin = f;
    }

    if (const juce::File file = skin.getChildFile(imageName + ".png"); file.exists())
        return juce::ImageCache::getFromFile(file);

    int size = 0;
    const char *data = BinaryData::getNamedResource((imageName + "_png").toUTF8(), size);
    return juce::ImageCache::getFromMemory(data, size);
}

void ScalableComponent::scaleFactorChanged() {}